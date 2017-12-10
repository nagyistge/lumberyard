/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "StdAfx.h"
#include <Cry_Geo.h>
#include "DisplayContext.h"
#include "IRenderAuxGeom.h"
#include "IEditor.h"
#include "Include/IIconManager.h"
#include "Include/IDisplayViewport.h"

#include <I3DEngine.h>

#include <QPoint>

#define FREEZE_COLOR QColor(100, 100, 100)

//////////////////////////////////////////////////////////////////////////
DisplayContext::DisplayContext()
{
    view = 0;
    renderer = 0;
    engine = 0;
    flags = 0;
    settings = 0;
    pIconManager = 0;
    m_renderState = 0;

    m_currentMatrix = 0;
    m_matrixStack[m_currentMatrix].SetIdentity();
    pRenderAuxGeom = gEnv->pRenderer->GetIRenderAuxGeom();
    m_thickness = 0;

    m_width = 0;
    m_height = 0;

    m_textureLabels.reserve(100);
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::SetView(IDisplayViewport* pView)
{
    view = pView;
    int width, height;
    view->GetDimensions(&width, &height);
    m_width = static_cast<float>(width);
    m_height = static_cast<float>(height);
    m_textureLabels.resize(0);
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::InternalDrawLine(const Vec3& v0, const ColorB& colV0, const Vec3& v1, const ColorB& colV1)
{
    pRenderAuxGeom->DrawLine(v0, colV0, v1, colV1, m_thickness);
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawPoint(const Vec3& p, int nSize)
{
    pRenderAuxGeom->DrawPoint(ToWorldSpacePosition(p), m_color4b, nSize);
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawTri(const Vec3& p1, const Vec3& p2, const Vec3& p3)
{
    pRenderAuxGeom->DrawTriangle(ToWorldSpacePosition(p1), m_color4b, ToWorldSpacePosition(p2), m_color4b, ToWorldSpacePosition(p3), m_color4b);
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawQuad(const Vec3& p1, const Vec3& p2, const Vec3& p3, const Vec3& p4)
{
    Vec3 p[4] = { ToWorldSpacePosition(p1), ToWorldSpacePosition(p2), ToWorldSpacePosition(p3), ToWorldSpacePosition(p4) };
    pRenderAuxGeom->DrawTriangle(p[0], m_color4b, p[1], m_color4b, p[2], m_color4b);
    pRenderAuxGeom->DrawTriangle(p[2], m_color4b, p[3], m_color4b, p[0], m_color4b);
    //pRenderAuxGeom->DrawPolyline( poly,4,true,m_color4b );
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawCylinder(const Vec3& p1, const Vec3& p2, float radius, float height)
{
    Vec3 p[2] = { ToWorldSpacePosition(p1), ToWorldSpacePosition(p2) };
    Vec3 dir = p[1] - p[0];
    pRenderAuxGeom->DrawCylinder(p[0], dir, radius, height, m_color4b);
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawCone(const Vec3& pos, const Vec3& dir, float radius, float height)
{
    const Vec3 worldPos = ToWorldSpacePosition(pos);
    const Vec3 worldDir = ToWorldSpaceVector(dir);
    pRenderAuxGeom->DrawCone(worldPos, worldDir, radius, height, m_color4b);
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawWireCylinder(const Vec3& center, const Vec3& axis, float radius, float height)
{
    if (radius > FLT_EPSILON && height > FLT_EPSILON && axis.GetLengthSquared() > FLT_EPSILON)
    {
        Vec3 axisNormalized = axis.GetNormalized();

        // Draw circles at bottom & top of cylinder
        Vec3 centerToTop = axisNormalized * height * 0.5f;
        Vec3 circle1Center = center - centerToTop;
        Vec3 circle2Center = center + centerToTop;
        // DrawArc() takes local coordinates
        DrawArc(circle1Center, radius, 0.0f, 360.0f, 22.5f, axisNormalized);
        DrawArc(circle2Center, radius, 0.0f, 360.0f, 22.5f, axisNormalized);

        // Draw 4 lines up side of cylinder
        Vec3 rightDirNormalized, frontDirNormalized;
        GetBasisVectors(axisNormalized, rightDirNormalized, frontDirNormalized);
        Vec3 centerToRightEdge = rightDirNormalized * radius;
        Vec3 centerToFrontEdge = frontDirNormalized * radius;
        // InternalDrawLine() takes world coordinates
        InternalDrawLine(ToWorldSpacePosition(circle1Center + centerToRightEdge), m_color4b,
            ToWorldSpacePosition(circle2Center + centerToRightEdge), m_color4b);
        InternalDrawLine(ToWorldSpacePosition(circle1Center - centerToRightEdge), m_color4b,
            ToWorldSpacePosition(circle2Center - centerToRightEdge), m_color4b);
        InternalDrawLine(ToWorldSpacePosition(circle1Center + centerToFrontEdge), m_color4b,
            ToWorldSpacePosition(circle2Center + centerToFrontEdge), m_color4b);
        InternalDrawLine(ToWorldSpacePosition(circle1Center - centerToFrontEdge), m_color4b,
            ToWorldSpacePosition(circle2Center - centerToFrontEdge), m_color4b);
    }
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawSolidCylinder(const Vec3& center, const Vec3& axis, float radius, float height)
{
    if (radius > FLT_EPSILON && height > FLT_EPSILON && axis.GetLengthSquared() > FLT_EPSILON)
    {
        // transform everything to world space
        Vec3 wsCenter = ToWorldSpacePosition(center);

        // determine scale in dir direction, apply to height
        Vec3 axisNormalized = axis.GetNormalized();
        Vec3 wsAxis = ToWorldSpaceVector(axisNormalized);
        float wsAxisLength = wsAxis.GetLength();
        float wsHeight = height * wsAxisLength;

        // determine scale in orthogonal direction, apply to radius
        Vec3 radiusDirNormalized = axisNormalized.GetOrthogonal();
        radiusDirNormalized.Normalize();
        Vec3 wsRadiusDir = ToWorldSpaceVector(radiusDirNormalized);
        float wsRadiusDirLen = wsRadiusDir.GetLength();
        float wsRadius = radius * wsRadiusDirLen;

        pRenderAuxGeom->DrawCylinder(wsCenter, wsAxis, wsRadius, wsHeight, m_color4b);
    }
}

//////////////////////////////////////////////////////////////////////////

void DisplayContext::DrawWireCapsule(const Vec3& center, const Vec3& axis, float radius, float heightStraightSection)
{
    if (radius > FLT_EPSILON && axis.GetLengthSquared() > FLT_EPSILON)
    {
        Vec3 axisNormalized = axis.GetNormalizedFast();

        // Draw cylinder part (or just a circle around the middle)
        if (heightStraightSection > FLT_EPSILON)
        {
            DrawWireCylinder(center, axis, radius, heightStraightSection);
        }
        else
        {
            DrawArc(center, radius, 0.0f, 360.0f, 22.5f, axisNormalized);
        }

        // Draw top cap as two criss-crossing 180deg arcs
        Vec3 ortho1Normalized, ortho2Normalized;
        GetBasisVectors(axisNormalized, ortho1Normalized, ortho2Normalized);
        Vec3 centerToTopCircleCenter = axisNormalized * heightStraightSection * 0.5f;
        DrawArc(center + centerToTopCircleCenter, radius,  90.0f, 180.0f, 22.5f, ortho1Normalized);
        DrawArc(center + centerToTopCircleCenter, radius, 180.0f, 180.0f, 22.5f, ortho2Normalized);

        // Draw bottom cap
        DrawArc(center - centerToTopCircleCenter, radius, -90.0f, 180.0f, 22.5f, ortho1Normalized);
        DrawArc(center - centerToTopCircleCenter, radius,   0.0f, 180.0f, 22.5f, ortho2Normalized);
    }
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawWireBox(const Vec3& min, const Vec3& max)
{
    pRenderAuxGeom->DrawAABB(AABB(min, max), m_matrixStack[m_currentMatrix], false, m_color4b, eBBD_Faceted);
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawSolidBox(const Vec3& min, const Vec3& max)
{
    pRenderAuxGeom->DrawAABB(AABB(min, max), m_matrixStack[m_currentMatrix], true, m_color4b, eBBD_Faceted);
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawSolidOBB(const Vec3& center, const Vec3& axisX, const Vec3& axisY, const Vec3& axisZ, const Vec3& halfExtents)
{
    OBB obb;
    obb.m33 = Matrix33::CreateFromVectors(axisX, axisY, axisZ);
    obb.c = Vec3(0, 0, 0);
    obb.h = halfExtents;
    pRenderAuxGeom->DrawOBB(obb, center, true, m_color4b, eBBD_Faceted);
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawLine(const Vec3& p1, const Vec3& p2)
{
    InternalDrawLine(ToWorldSpacePosition(p1), m_color4b, ToWorldSpacePosition(p2), m_color4b);
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawPolyLine(const Vec3* pnts, int numPoints, bool cycled)
{
    MAKE_SURE(numPoints >= 2, return );
    MAKE_SURE(pnts != 0, return );

    int numSegments = cycled ? numPoints : (numPoints - 1);
    Vec3 p1 = ToWorldSpacePosition(pnts[0]);
    Vec3 p2;
    for (int i = 0; i < numSegments; i++)
    {
        int j = (i + 1) % numPoints;
        p2 = ToWorldSpacePosition(pnts[j]);
        InternalDrawLine(p1, m_color4b, p2, m_color4b);
        p1 = p2;
    }
}

//////////////////////////////////////////////////////////////////////////
float DisplayContext::GetWaterLevelAtPos(const Vec3& vPos) const
{
    float fWaterLevel = engine->GetWaterLevel(&vPos);
    float fOceanLevel = engine->GetAccurateOceanHeight(vPos);

    if (fWaterLevel != WATER_LEVEL_UNKNOWN && fWaterLevel != fOceanLevel)
    {
        return fWaterLevel;
    }

    return WATER_LEVEL_UNKNOWN;
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawTerrainCircle(const Vec3& worldPos, float radius, float height)
{
    // Draw circle with default radius.
    Vec3 p0, p1;
    p0.x = worldPos.x + radius * sin(0.0f);
    p0.y = worldPos.y + radius * cos(0.0f);
    p0.z = engine->GetTerrainElevation(p0.x, p0.y) + height;

    float step = 20.0f / 180 * gf_PI;
    for (float angle = step; angle < 360.0f / 180 * gf_PI + step; angle += step)
    {
        p1.x = worldPos.x + radius * sin(angle);
        p1.y = worldPos.y + radius * cos(angle);
        p1.z = engine->GetTerrainElevation(p1.x, p1.y) + height;

        InternalDrawLine(ToWorldSpacePosition(p0), m_color4b, ToWorldSpacePosition(p1), m_color4b);
        p0 = p1;
    }
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawTerrainCircle(const Vec3& worldPos, float radius, float angle1, float angle2, float height)
{
    // Draw circle with default radius.
    Vec3 p0, p1;
    p0.x = worldPos.x + radius * sin(angle1);
    p0.y = worldPos.y + radius * cos(angle1);
    p0.z = engine->GetTerrainElevation(p0.x, p0.y) + height;

    float step = 20.0f / 180 * gf_PI;
    for (float angle = step + angle1; angle < angle2; angle += step)
    {
        p1.x = worldPos.x + radius * sin(angle);
        p1.y = worldPos.y + radius * cos(angle);
        p1.z = engine->GetTerrainElevation(p1.x, p1.y) + height;

        InternalDrawLine(ToWorldSpacePosition(p0), m_color4b, ToWorldSpacePosition(p1), m_color4b);
        p0 = p1;
    }

    p1.x = worldPos.x + radius * sin(angle2);
    p1.y = worldPos.y + radius * cos(angle2);
    p1.z = engine->GetTerrainElevation(p1.x, p1.y) + height;

    InternalDrawLine(ToWorldSpacePosition(p0), m_color4b, ToWorldSpacePosition(p1), m_color4b);
}


void DisplayContext::DrawArc(const Vec3& pos, float radius, float startAngleDegrees, float sweepAngleDegrees, float angularStepDegrees, int referenceAxis)
{
    // angularStepDegrees cannot be zero as it is used to divide sweepAngleDegrees
    if (fabs(angularStepDegrees) < FLT_EPSILON)
    {
        return;
    }

    float startAngleRadians = DEG2RAD(startAngleDegrees);
    float sweepAngleRadians = DEG2RAD(sweepAngleDegrees);

    float angle = startAngleRadians;

    const uint referenceAxis0 = referenceAxis % 3;
    const uint referenceAxis1 = (referenceAxis + 1) % 3;
    const uint referenceAxis2 = (referenceAxis + 2) % 3;

    Vec3 p0;
    p0[referenceAxis0] = pos[referenceAxis0];
    p0[referenceAxis1] = pos[referenceAxis1] + radius * sin(angle);
    p0[referenceAxis2] = pos[referenceAxis2] + radius * cos(angle);
    p0 = ToWorldSpacePosition(p0);

    float step = DEG2RAD(angularStepDegrees);
    int numSteps = std::abs(static_cast<int>(std::ceil(sweepAngleRadians / step)));

    Vec3 p1;
    for (int i = 0; i < numSteps; ++i)
    {
        angle += std::min(step, sweepAngleRadians); // Don't go past sweepAngleRadians when stepping or the arc will be too long.
        sweepAngleRadians -= step;

        p1[referenceAxis0] = pos[referenceAxis0];
        p1[referenceAxis1] = pos[referenceAxis1] + radius * sin(angle);
        p1[referenceAxis2] = pos[referenceAxis2] + radius * cos(angle);
        p1 = ToWorldSpacePosition(p1);

        InternalDrawLine(p0, m_color4b, p1, m_color4b);
        p0 = p1;
    }
}

void DisplayContext::DrawArc(const Vec3& pos, float radius, float startAngleDegrees, float sweepAngleDegrees, float angularStepDegrees, const Vec3& fixedAxis)
{
    // angularStepDegrees cannot be zero as it is used to divide sweepAngleDegrees
    if (fabs(angularStepDegrees) < FLT_EPSILON)
    {
        return;
    }

    float startAngleRadians = DEG2RAD(startAngleDegrees);
    float sweepAngleRadians = DEG2RAD(sweepAngleDegrees);

    Vec3 a;
    Vec3 b;
    GetBasisVectors(fixedAxis, a, b);

    float angle = startAngleRadians;

    float cosAngle = cos(angle) * radius;
    float sinAngle = sin(angle) * radius;

    Vec3 p0;
    p0.x = pos.x + cosAngle * a.x + sinAngle * b.x;
    p0.y = pos.y + cosAngle * a.y + sinAngle * b.y;
    p0.z = pos.z + cosAngle * a.z + sinAngle * b.z;
    p0 = ToWorldSpacePosition(p0);

    float step = DEG2RAD(angularStepDegrees);
    int numSteps = std::abs(static_cast<int>(std::ceil(sweepAngleRadians / step)));

    Vec3 p1;
    for (int i = 0; i < numSteps; ++i)
    {
        angle += std::min(step, sweepAngleRadians); // Don't go past sweepAngleRadians when stepping or the arc will be too long.
        sweepAngleRadians -= step;

        float cosAngle = cos(angle) * radius;
        float sinAngle = sin(angle) * radius;

        p1.x = pos.x + cosAngle * a.x + sinAngle * b.x;
        p1.y = pos.y + cosAngle * a.y + sinAngle * b.y;
        p1.z = pos.z + cosAngle * a.z + sinAngle * b.z;
        p1 = ToWorldSpacePosition(p1);

        InternalDrawLine(p0, m_color4b, p1, m_color4b);

        p0 = p1;
    }
}

//////////////////////////////////////////////////////////////////////////
// Vera, Confetti
void DisplayContext::DrawArcWithArrow(const Vec3& pos, float radius, float startAngleDegrees, float sweepAngleDegrees, float angularStepDegrees, const Vec3& fixedAxis)
{
    // angularStepDegrees cannot be zero as it is used to divide sweepAngleDegrees
    // Grab the code from draw arc to get the final p0 and p1;
    if (fabs(angularStepDegrees) < FLT_EPSILON)
    {
        return;
    }

    float startAngleRadians = DEG2RAD(startAngleDegrees);
    float sweepAngleRadians = DEG2RAD(sweepAngleDegrees);

    Vec3 a;
    Vec3 b;
    GetBasisVectors(fixedAxis, a, b);

    float angle = startAngleRadians;

    float cosAngle = cos(angle) * radius;
    float sinAngle = sin(angle) * radius;

    Vec3 p0;
    p0.x = pos.x + cosAngle * a.x + sinAngle * b.x;
    p0.y = pos.y + cosAngle * a.y + sinAngle * b.y;
    p0.z = pos.z + cosAngle * a.z + sinAngle * b.z;
    p0 = ToWorldSpacePosition(p0);

    float step = DEG2RAD(angularStepDegrees);
    int numSteps = std::abs(static_cast<int>(std::ceil(sweepAngleRadians / step)));

    Vec3 p1;
    for (int i = 0; i < numSteps; ++i)
    {
        angle += step;
        float cosAngle = cos(angle) * radius;
        float sinAngle = sin(angle) * radius;

        p1.x = pos.x + cosAngle * a.x + sinAngle * b.x;
        p1.y = pos.y + cosAngle * a.y + sinAngle * b.y;
        p1.z = pos.z + cosAngle * a.z + sinAngle * b.z;
        p1 = ToWorldSpacePosition(p1);

        if (i + 1 >= numSteps) // For last step, draw an arrow
        {
            // p0 and p1 are global position. We could like to map it to local
            Matrix34 inverseMat =  m_matrixStack[m_currentMatrix].GetInverted();
            Vec3 localP0 = inverseMat.TransformPoint(p0);
            Vec3 localP1 = inverseMat.TransformPoint(p1);
            DrawArrow(localP0, localP1, m_thickness);
        }
        else
        {
            InternalDrawLine(p0, m_color4b, p1, m_color4b);
        }

        p0 = p1;
    }
}


//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawCircle(const Vec3& pos, float radius, int nUnchangedAxis)
{
    // Draw circle with default radius.
    Vec3 p0, p1;
    p0[nUnchangedAxis] = pos[nUnchangedAxis];
    p0[(nUnchangedAxis + 1) % 3] = pos[(nUnchangedAxis + 1) % 3] + radius * sin(0.0f);
    p0[(nUnchangedAxis + 2) % 3] = pos[(nUnchangedAxis + 2) % 3] + radius * cos(0.0f);
    p0 = ToWorldSpacePosition(p0);
    float step = 10.0f / 180 * gf_PI;
    for (float angle = step; angle < 360.0f / 180 * gf_PI + step; angle += step)
    {
        p1[nUnchangedAxis] = pos[nUnchangedAxis];
        p1[(nUnchangedAxis + 1) % 3] = pos[(nUnchangedAxis + 1) % 3] + radius * sin(angle);
        p1[(nUnchangedAxis + 2) % 3] = pos[(nUnchangedAxis + 2) % 3] + radius * cos(angle);
        p1 = ToWorldSpacePosition(p1);
        InternalDrawLine(p0, m_color4b, p1, m_color4b);
        p0 = p1;
    }
}

//////////////////////////////////////////////////////////////////////////
// Vera, Confetti
void DisplayContext::DrawDottedCircle(const Vec3& pos, float radius, const Vec3& nUnchangedAxis, int numberOfArrows /*=0*/, float stepDegree /*= 1*/)
{
    float startAngleRadians = 0;
    float sweepAngleRadians = 2 * gf_PI;

    Vec3 a;
    Vec3 b;
    GetBasisVectors(nUnchangedAxis, a, b);

    float angle = startAngleRadians;

    float cosAngle = cos(angle) * radius;
    float sinAngle = sin(angle) * radius;

    Vec3 p0;
    p0.x = pos.x + cosAngle * a.x + sinAngle * b.x;
    p0.y = pos.y + cosAngle * a.y + sinAngle * b.y;
    p0.z = pos.z + cosAngle * a.z + sinAngle * b.z;
    p0 = ToWorldSpacePosition(p0);

    float step = DEG2RAD(stepDegree); // one degree each step
    int numSteps = 2.0f * gf_PI / step;

    // Prepare for drawing arrow
    float arrowStep = 0;
    float arrowAngle = 0;
    if (numberOfArrows > 0)
    {
        arrowStep = 2 * gf_PI / numberOfArrows;
        arrowAngle = arrowStep;
    }

    Vec3 p1;
    for (int i = 0; i < numSteps; ++i)
    {
        angle += step;
        float cosAngle = cos(angle) * radius;
        float sinAngle = sin(angle) * radius;

        p1.x = pos.x + cosAngle * a.x + sinAngle * b.x;
        p1.y = pos.y + cosAngle * a.y + sinAngle * b.y;
        p1.z = pos.z + cosAngle * a.z + sinAngle * b.z;
        p1 = ToWorldSpacePosition(p1);


        if (arrowStep > 0) // If user want to draw arrow on circle
        {
            // if the arraw should be drawn between current angel and next angel
            if (angle <= arrowAngle && angle + step * 2 > arrowAngle)
            {
                // Get local position from global position
                Matrix34 inverseMat = m_matrixStack[m_currentMatrix].GetInverted();
                Vec3 localP0 = inverseMat.TransformPoint(p0);
                Vec3 localP1 = inverseMat.TransformPoint(p1);
                DrawArrow(localP0, localP1, m_thickness);
                arrowAngle += arrowStep;
                if (arrowAngle > 2 * gf_PI) // if the next arrow angle is greater than 2PI. Stop adding arrow.
                {
                    arrowStep = 0;
                }
            }
        }

        InternalDrawLine(p0, m_color4b, p1, m_color4b);

        // Skip a step
        angle += step;
        cosAngle = cos(angle) * radius;
        sinAngle = sin(angle) * radius;

        p1.x = pos.x + cosAngle * a.x + sinAngle * b.x;
        p1.y = pos.y + cosAngle * a.y + sinAngle * b.y;
        p1.z = pos.z + cosAngle * a.z + sinAngle * b.z;
        p1 = ToWorldSpacePosition(p1);

        p0 = p1;
    }
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawWireCircle2d(const QPoint& center, float radius, float z)
{
    Vec3 p0, p1, pos;
    pos.x = static_cast<float>(center.x());
    pos.y = static_cast<float>(center.y());
    pos.z = z;
    p0.x = pos.x + radius * sin(0.0f);
    p0.y = pos.y + radius * cos(0.0f);
    p0.z = z;
    float step = 10.0f / 180 * gf_PI;

    int prevState = GetState();
    //SetState( (prevState|e_Mode2D) & (~e_Mode3D) );
    for (float angle = step; angle < 360.0f / 180 * gf_PI + step; angle += step)
    {
        p1.x = pos.x + radius * sin(angle);
        p1.y = pos.y + radius * cos(angle);
        p1.z = z;
        InternalDrawLine(p0, m_color4b, p1, m_color4b);
        p0 = p1;
    }
    SetState(prevState);
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawWireSphere(const Vec3& pos, float radius)
{
    Vec3 p0, p1;
    float step = 10.0f / 180 * gf_PI;
    float angle;

    // Z Axis
    p0 = pos;
    p1 = pos;
    p0.x += radius * sin(0.0f);
    p0.y += radius * cos(0.0f);
    p0 = ToWorldSpacePosition(p0);
    for (angle = step; angle < 360.0f / 180 * gf_PI + step; angle += step)
    {
        p1.x = pos.x + radius * sin(angle);
        p1.y = pos.y + radius * cos(angle);
        p1.z = pos.z;
        p1 = ToWorldSpacePosition(p1);
        InternalDrawLine(p0, m_color4b, p1, m_color4b);
        p0 = p1;
    }

    // X Axis
    p0 = pos;
    p1 = pos;
    p0.y += radius * sin(0.0f);
    p0.z += radius * cos(0.0f);
    p0 = ToWorldSpacePosition(p0);
    for (angle = step; angle < 360.0f / 180 * gf_PI + step; angle += step)
    {
        p1.x = pos.x;
        p1.y = pos.y + radius * sin(angle);
        p1.z = pos.z + radius * cos(angle);
        p1 = ToWorldSpacePosition(p1);
        InternalDrawLine(p0, m_color4b, p1, m_color4b);
        p0 = p1;
    }

    // Y Axis
    p0 = pos;
    p1 = pos;
    p0.x += radius * sin(0.0f);
    p0.z += radius * cos(0.0f);
    p0 = ToWorldSpacePosition(p0);
    for (angle = step; angle < 360.0f / 180 * gf_PI + step; angle += step)
    {
        p1.x = pos.x + radius * sin(angle);
        p1.y = pos.y;
        p1.z = pos.z + radius * cos(angle);
        p1 = ToWorldSpacePosition(p1);
        InternalDrawLine(p0, m_color4b, p1, m_color4b);
        p0 = p1;
    }
}


//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawWireSphere(const Vec3& pos, const Vec3 radius)
{
    Vec3 p0, p1;
    float step = 10.0f / 180 * gf_PI;
    float angle;

    // Z Axis
    p0 = pos;
    p1 = pos;
    p0.x += radius.x * sin(0.0f);
    p0.y += radius.y * cos(0.0f);
    p0 = ToWorldSpacePosition(p0);
    for (angle = step; angle < 360.0f / 180 * gf_PI + step; angle += step)
    {
        p1.x = pos.x + radius.x * sin(angle);
        p1.y = pos.y + radius.y * cos(angle);
        p1.z = pos.z;
        p1 = ToWorldSpacePosition(p1);
        InternalDrawLine(p0, m_color4b, p1, m_color4b);
        p0 = p1;
    }

    // X Axis
    p0 = pos;
    p1 = pos;
    p0.y += radius.y * sin(0.0f);
    p0.z += radius.z * cos(0.0f);
    p0 = ToWorldSpacePosition(p0);
    for (angle = step; angle < 360.0f / 180 * gf_PI + step; angle += step)
    {
        p1.x = pos.x;
        p1.y = pos.y + radius.y * sin(angle);
        p1.z = pos.z + radius.z * cos(angle);
        p1 = ToWorldSpacePosition(p1);
        InternalDrawLine(p0, m_color4b, p1, m_color4b);
        p0 = p1;
    }

    // Y Axis
    p0 = pos;
    p1 = pos;
    p0.x += radius.x * sin(0.0f);
    p0.z += radius.z * cos(0.0f);
    p0 = ToWorldSpacePosition(p0);
    for (angle = step; angle < 360.0f / 180 * gf_PI + step; angle += step)
    {
        p1.x = pos.x + radius.x * sin(angle);
        p1.y = pos.y;
        p1.z = pos.z + radius.z * cos(angle);
        p1 = ToWorldSpacePosition(p1);
        InternalDrawLine(p0, m_color4b, p1, m_color4b);
        p0 = p1;
    }
}

void DisplayContext::DrawWireQuad2d(const QPoint& pmin, const QPoint& pmax, float z)
{
    int prevState = GetState();
    SetState((prevState | e_Mode2D) & (~e_Mode3D));
    float minX = static_cast<float>(pmin.x());
    float minY = static_cast<float>(pmin.y());
    float maxX = static_cast<float>(pmax.x());
    float maxY = static_cast<float>(pmax.y());
    InternalDrawLine(Vec3(minX, minY, z), m_color4b, Vec3(maxX, minY, z), m_color4b);
    InternalDrawLine(Vec3(maxX, minY, z), m_color4b, Vec3(maxX, maxY, z), m_color4b);
    InternalDrawLine(Vec3(maxX, maxY, z), m_color4b, Vec3(minX, maxY, z), m_color4b);
    InternalDrawLine(Vec3(minX, maxY, z), m_color4b, Vec3(minX, minY, z), m_color4b);
    SetState(prevState);
}

void DisplayContext::DrawLine2d(const QPoint& p1, const QPoint& p2, float z)
{
    int prevState = GetState();

    SetState((prevState | e_Mode2D) & (~e_Mode3D));

    // If we don't have correct information, we try to get it, but while we
    // don't, we skip rendering this frame.
    if (m_width == 0 || m_height == 0)
    {
        if (view)
        {
            // We tell the window to update itself, as it might be needed to
            // get correct information.
            view->Update();
            int width, height;
            view->GetDimensions(&width, &height);
            m_width = static_cast<float>(width);
            m_height = static_cast<float>(height);
        }
    }
    else
    {
        InternalDrawLine(Vec3(p1.x() / m_width, p1.y() / m_height, z), m_color4b, Vec3(p2.x() / m_width, p2.y() / m_height, z), m_color4b);
    }

    SetState(prevState);
}

void DisplayContext::DrawLine2dGradient(const QPoint& p1, const QPoint& p2, float z, ColorB firstColor, ColorB secondColor)
{
    int prevState = GetState();

    SetState((prevState | e_Mode2D) & (~e_Mode3D));
    InternalDrawLine(Vec3(p1.x() / m_width, p1.y() / m_height, z), firstColor, Vec3(p2.x() / m_width, p2.y() / m_height, z), secondColor);
    SetState(prevState);
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawQuadGradient(const Vec3& p1, const Vec3& p2, const Vec3& p3, const Vec3& p4, ColorB firstColor, ColorB secondColor)
{
    Vec3 p[4] = { ToWorldSpacePosition(p1), ToWorldSpacePosition(p2), ToWorldSpacePosition(p3), ToWorldSpacePosition(p4) };
    pRenderAuxGeom->DrawTriangle(p[0], firstColor, p[1], firstColor, p[2], secondColor);
    pRenderAuxGeom->DrawTriangle(p[2], secondColor, p[3], secondColor, p[0], firstColor);
}

//////////////////////////////////////////////////////////////////////////
QColor DisplayContext::GetSelectedColor()
{
    float t = GetTickCount() / 1000.0f;
    float r1 = fabs(sin(t * 8.0f));
    if (r1 > 255)
    {
        r1 = 255;
    }
    return QColor(255, 0, r1 * 255);
    //          float r2 = cos(t*3);
    //dc.renderer->SetMaterialColor( 1,0,r1,0.5f );
}

QColor DisplayContext::GetFreezeColor()
{
    return FREEZE_COLOR;
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::SetSelectedColor(float fAlpha)
{
    SetColor(GetSelectedColor(), fAlpha);
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::SetFreezeColor()
{
    SetColor(FREEZE_COLOR, 0.5f);
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawLine(const Vec3& p1, const Vec3& p2, const ColorF& col1, const ColorF& col2)
{
    InternalDrawLine(ToWorldSpacePosition(p1), ColorB(col1), ToWorldSpacePosition(p2), ColorB(col2));
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawLine(const Vec3& p1, const Vec3& p2, const QColor& rgb1, const QColor& rgb2)
{
    InternalDrawLine(ToWorldSpacePosition(p1), ColorB(rgb1.red(), rgb1.green(), rgb1.blue(), 255), ToWorldSpacePosition(p2), ColorB(rgb2.red(), rgb2.green(), rgb2.blue(), 255));
}

//////////////////////////////////////////////////////////////////////////
// Vera, Confetti
void DisplayContext::DrawDottedLine(const Vec3& p1, const Vec3& p2, const ColorF& col1, const ColorF& col2, const float numOfSteps)
{
    Vec3 direction =  Vec3(p2.x - p1.x, p2.y - p1.y, p2.z - p1.z);
    //We only draw half of a step and leave the other half empty to make it a dotted line.
    Vec3 halfstep = (direction / numOfSteps) * 0.5f; 
    Vec3 startPoint = p1;

    for (int stepCount = 0; stepCount < numOfSteps; stepCount++)
    {
        InternalDrawLine(ToWorldSpacePosition(startPoint), m_color4b, ToWorldSpacePosition(startPoint + halfstep), m_color4b);
        startPoint += 2 * halfstep; //skip a half step to make it dotted
    }
}


//////////////////////////////////////////////////////////////////////////
void DisplayContext::PushMatrix(const Matrix34& tm)
{
    assert(m_currentMatrix < 32);
    if (m_currentMatrix < 32)
    {
        m_currentMatrix++;
        m_matrixStack[m_currentMatrix] = m_matrixStack[m_currentMatrix - 1] * tm;
    }
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::PopMatrix()
{
    assert(m_currentMatrix > 0);
    if (m_currentMatrix > 0)
    {
        m_currentMatrix--;
    }
}

//////////////////////////////////////////////////////////////////////////
const Matrix34& DisplayContext::GetMatrix()
{
    return m_matrixStack[m_currentMatrix];
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawBall(const Vec3& pos, float radius)
{
    pRenderAuxGeom->DrawSphere(ToWorldSpacePosition(pos), radius, m_color4b);
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawArrow(const Vec3& src, const Vec3& trg, float fHeadScale, bool b2SidedArrow)
{
    float f2dScale = 1.0f;
    float arrowLen = 0.4f * fHeadScale;
    float arrowRadius = 0.1f * fHeadScale;
    if (flags & DISPLAY_2D)
    {
        f2dScale = 1.2f * ToWorldSpaceVector(Vec3(1, 0, 0)).GetLength();
    }
    Vec3 dir = trg - src;
    dir = ToWorldSpaceVector(dir.GetNormalized());
    Vec3 p0 = ToWorldSpacePosition(src);
    Vec3 p1 = ToWorldSpacePosition(trg);
    if (!b2SidedArrow)
    {
        p1 = p1 - dir * arrowLen;
        InternalDrawLine(p0, m_color4b, p1, m_color4b);
        pRenderAuxGeom->DrawCone(p1, dir, arrowRadius * f2dScale, arrowLen * f2dScale, m_color4b);
    }
    else
    {
        p0 = p0 + dir * arrowLen;
        p1 = p1 - dir * arrowLen;
        InternalDrawLine(p0, m_color4b, p1, m_color4b);
        pRenderAuxGeom->DrawCone(p0, -dir, arrowRadius * f2dScale, arrowLen * f2dScale, m_color4b);
        pRenderAuxGeom->DrawCone(p1, dir, arrowRadius * f2dScale, arrowLen * f2dScale, m_color4b);
    }
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::RenderObject(int objectType, const Vec3& pos, float scale)
{
    Matrix34 tm;
    tm.SetIdentity();

    tm = Matrix33::CreateScale(Vec3(scale, scale, scale)) * tm;

    tm.SetTranslation(pos);
    RenderObject(objectType, tm);
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::RenderObject(int objectType, const Matrix34& tm)
{
    IStatObj* object = pIconManager ? pIconManager->GetObject((EStatObject)objectType) : 0;
    if (object)
    {
        float color[4];
        color[0] = m_color4b.r * (1.0f / 255.0f);
        color[1] = m_color4b.g * (1.0f / 255.0f);
        color[2] = m_color4b.b * (1.0f / 255.0f);
        color[3] = m_color4b.a * (1.0f / 255.0f);

        SRenderingPassInfo passInfo = SRenderingPassInfo::CreateGeneralPassRenderingInfo(GetIEditor()->GetSystem()->GetViewCamera());

        Matrix34 xform = m_matrixStack[m_currentMatrix] * tm;
        SRendParams rp;
        rp.pMatrix = &xform;
        rp.AmbientColor = ColorF(color[0], color[1], color[2], 1);
        rp.fAlpha = color[3];

        object->Render(rp, passInfo);
    }
}

/////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawTerrainRect(float x1, float y1, float x2, float y2, float height)
{
    Vec3 p1, p2;
    float x, y;

    float step = MAX(y2 - y1, x2 - x1);
    if (step < 0.1)
    {
        return;
    }
    step = step / 100.0f;
    if (step > 10)
    {
        step /= 10;
    }

    for (y = y1; y < y2; y += step)
    {
        float ye = min(y + step, y2);

        p1.x = x1;
        p1.y = y;
        p1.z = engine->GetTerrainElevation(p1.x, p1.y) + height;

        p2.x = x1;
        p2.y = ye;
        p2.z = engine->GetTerrainElevation(p2.x, p2.y) + height;
        DrawLine(p1, p2);

        p1.x = x2;
        p1.y = y;
        p1.z = engine->GetTerrainElevation(p1.x, p1.y) + height;

        p2.x = x2;
        p2.y = ye;
        p2.z = engine->GetTerrainElevation(p2.x, p2.y) + height;
        DrawLine(p1, p2);
    }
    for (x = x1; x < x2; x += step)
    {
        float xe = min(x + step, x2);

        p1.x = x;
        p1.y = y1;
        p1.z = engine->GetTerrainElevation(p1.x, p1.y) + height;

        p2.x = xe;
        p2.y = y1;
        p2.z = engine->GetTerrainElevation(p2.x, p2.y) + height;
        DrawLine(p1, p2);

        p1.x = x;
        p1.y = y2;
        p1.z = engine->GetTerrainElevation(p1.x, p1.y) + height;

        p2.x = xe;
        p2.y = y2;
        p2.z = engine->GetTerrainElevation(p2.x, p2.y) + height;
        DrawLine(p1, p2);
    }
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawTerrainLine(Vec3 worldPos1, Vec3 worldPos2)
{
    worldPos1.z = worldPos2.z = 0;

    int steps = static_cast<int>((worldPos2 - worldPos1).GetLength() / 4);
    if (steps == 0)
    {
        steps = 1;
    }

    Vec3 step = (worldPos2 - worldPos1) / static_cast<float>(steps);

    Vec3 p1 = worldPos1;
    p1.z = engine->GetTerrainElevation(worldPos1.x, worldPos1.y);
    for (int i = 0; i < steps; ++i)
    {
        Vec3 p2 = p1 + step;
        p2.z = 0.1f + engine->GetTerrainElevation(p2.x, p2.y);

        DrawLine(p1, p2);

        p1 = p2;
    }
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawTextLabel(const Vec3& pos, float size, const char* text, const bool bCenter, int srcOffsetX, int scrOffsetY)
{
    ColorF col(m_color4b.r * (1.0f / 255.0f), m_color4b.g * (1.0f / 255.0f), m_color4b.b * (1.0f / 255.0f), m_color4b.a * (1.0f / 255.0f));

    float fCol[4] = { col.r, col.g, col.b, col.a };
    if (flags & DISPLAY_2D)
    {
        //We need to pass Screen coordinates to Draw2dLabel Function
        Vec3 screenPos = GetView()->GetScreenTM().TransformPoint(pos);
        renderer->Draw2dLabel(screenPos.x, screenPos.y, size, fCol, bCenter, "%s", text);
    }
    else
    {
        renderer->DrawLabelEx(pos, size, fCol, true, true, text);
    }
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::Draw2dTextLabel(float x, float y, float size, const char* text, bool bCenter)
{
    float col[4] = { m_color4b.r * (1.0f / 255.0f), m_color4b.g * (1.0f / 255.0f), m_color4b.b * (1.0f / 255.0f), m_color4b.a * (1.0f / 255.0f) };
    renderer->Draw2dLabel(x, y, size, col, bCenter, "%s", text);
}

void DisplayContext::DrawTextOn2DBox(const Vec3& pos, const char* text, float textScale, const ColorF& TextColor, const ColorF& TextBackColor)
{
    Vec3 worldPos = ToWorldSpacePosition(pos);
    int vx, vy, vw, vh;
    gEnv->pRenderer->GetViewport(&vx, &vy, &vw, &vh);
    uint32 backupstate = GetState();

    SetState(backupstate | e_DepthTestOff);

    const CCamera& camera = gEnv->pRenderer->GetCamera();
    Vec3 screenPos;

    camera.Project(worldPos, screenPos, Vec2i(0, 0), Vec2i(0, 0));

    //! Font size information doesn't seem to exist so the proper size is used
    int textlen = strlen(text);
    float fontsize = 7.5f * textScale;
    float textwidth = fontsize * textlen;
    float textheight = 16.0f * textScale;

    screenPos.x = screenPos.x - (textwidth * 0.5f);

    Vec3 textregion[4] = {
        Vec3(screenPos.x, screenPos.y, screenPos.z),
        Vec3(screenPos.x + textwidth, screenPos.y, screenPos.z),
        Vec3(screenPos.x + textwidth, screenPos.y + textheight, screenPos.z),
        Vec3(screenPos.x, screenPos.y + textheight, screenPos.z)
    };

    Vec3 textworldreign[4];
    Matrix34 dcInvTm = GetMatrix().GetInverted();

    Matrix44A mProj, mView;
    mathMatrixPerspectiveFov(&mProj, camera.GetFov(), camera.GetProjRatio(), camera.GetNearPlane(), camera.GetFarPlane());
    mathMatrixLookAt(&mView, camera.GetPosition(), camera.GetPosition() + camera.GetViewdir(), Vec3(0, 0, 1));
    Matrix44A mInvViewProj = (mView * mProj).GetInverted();

    if (vw == 0)
    {
        vw = 1;
    }

    if (vh == 0)
    {
        vh = 1;
    }

    for (int i = 0; i < 4; ++i)
    {
        Vec4 projectedpos = Vec4((textregion[i].x - vx) / vw * 2.0f - 1.0f,
                -((textregion[i].y - vy) / vh) * 2.0f + 1.0f,
                textregion[i].z,
                1.0f);

        Vec4 wp = projectedpos * mInvViewProj;

        if (wp.w == 0.0f)
        {
            wp.w = 0.0001f;
        }

        wp.x /= wp.w;
        wp.y /= wp.w;
        wp.z /= wp.w;
        textworldreign[i] = dcInvTm.TransformPoint(Vec3(wp.x, wp.y, wp.z));
    }

    ColorB backupcolor = GetColor();

    SetColor(TextBackColor);
    SetDrawInFrontMode(true);
    DrawQuad(textworldreign[3], textworldreign[2], textworldreign[1], textworldreign[0]);
    SetColor(TextColor);
    DrawTextLabel(pos, textScale, text);
    SetDrawInFrontMode(false);
    SetColor(backupcolor);
    SetState(backupstate);
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::SetLineWidth(float width)
{
    m_thickness = width;
}

//////////////////////////////////////////////////////////////////////////
bool DisplayContext::IsVisible(const AABB& bounds)
{
    if (flags & DISPLAY_2D)
    {
        if (box.IsIntersectBox(bounds))
        {
            return true;
        }
    }
    else
    {
        return camera->IsAABBVisible_F(AABB(bounds.min, bounds.max));
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
uint32 DisplayContext::GetState() const
{
    return m_renderState;
}

//! Set a new render state flags.
//! @param returns previous render state.
uint32 DisplayContext::SetState(uint32 state)
{
    uint32 old = m_renderState;
    m_renderState = state;
    pRenderAuxGeom->SetRenderFlags(m_renderState);
    return old;
}

//! Set a new render state flags.
//! @param returns previous render state.
uint32 DisplayContext::SetStateFlag(uint32 state)
{
    uint32 old = m_renderState;
    m_renderState |= state;
    m_renderState = pRenderAuxGeom->GetRenderFlags().m_renderFlags;
    pRenderAuxGeom->SetRenderFlags(m_renderState);
    return old;
}

//! Clear specified flags in render state.
//! @param returns previous render state.
uint32 DisplayContext::ClearStateFlag(uint32 state)
{
    uint32 old = m_renderState;
    m_renderState &= ~state;
    m_renderState = pRenderAuxGeom->GetRenderFlags().m_renderFlags;
    pRenderAuxGeom->SetRenderFlags(m_renderState);
    return old;
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DepthTestOff()
{
    m_renderState = pRenderAuxGeom->GetRenderFlags().m_renderFlags;
    pRenderAuxGeom->SetRenderFlags((m_renderState | e_DepthTestOff) & (~e_DepthTestOn));
    m_renderState = pRenderAuxGeom->GetRenderFlags().m_renderFlags;
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DepthTestOn()
{
    m_renderState = pRenderAuxGeom->GetRenderFlags().m_renderFlags;
    pRenderAuxGeom->SetRenderFlags((m_renderState | e_DepthTestOn) & (~e_DepthTestOff));
    m_renderState = pRenderAuxGeom->GetRenderFlags().m_renderFlags;
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DepthWriteOff()
{
    m_renderState = pRenderAuxGeom->GetRenderFlags().m_renderFlags;
    pRenderAuxGeom->SetRenderFlags((m_renderState | e_DepthWriteOff) & (~e_DepthWriteOn));
    m_renderState = pRenderAuxGeom->GetRenderFlags().m_renderFlags;
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DepthWriteOn()
{
    m_renderState = pRenderAuxGeom->GetRenderFlags().m_renderFlags;
    pRenderAuxGeom->SetRenderFlags((m_renderState | e_DepthWriteOn) & (~e_DepthWriteOff));
    m_renderState = pRenderAuxGeom->GetRenderFlags().m_renderFlags;
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::CullOff()
{
    m_renderState = pRenderAuxGeom->GetRenderFlags().m_renderFlags;
    pRenderAuxGeom->SetRenderFlags((m_renderState | e_CullModeNone) & (~(e_CullModeBack | e_CullModeFront)));
    m_renderState = pRenderAuxGeom->GetRenderFlags().m_renderFlags;
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::CullOn()
{
    m_renderState = pRenderAuxGeom->GetRenderFlags().m_renderFlags;
    pRenderAuxGeom->SetRenderFlags((m_renderState | e_CullModeBack) & (~(e_CullModeNone | e_CullModeFront)));
    m_renderState = pRenderAuxGeom->GetRenderFlags().m_renderFlags;
}

//////////////////////////////////////////////////////////////////////////
bool DisplayContext::SetDrawInFrontMode(bool bOn)
{
    int prevState = m_renderState;
    SAuxGeomRenderFlags renderFlags = m_renderState;
    renderFlags.SetDrawInFrontMode((bOn) ? e_DrawInFrontOn : e_DrawInFrontOff);
    pRenderAuxGeom->SetRenderFlags(renderFlags);
    m_renderState = pRenderAuxGeom->GetRenderFlags().m_renderFlags;
    return (prevState & e_DrawInFrontOn) != 0;
}

int DisplayContext::SetFillMode(int nFillMode)
{
    int prevState = m_renderState;
    SAuxGeomRenderFlags renderFlags = m_renderState;
    renderFlags.SetFillMode((EAuxGeomPublicRenderflags_FillMode)nFillMode);
    pRenderAuxGeom->SetRenderFlags(renderFlags);
    m_renderState = pRenderAuxGeom->GetRenderFlags().m_renderFlags;
    return prevState;
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::DrawTextureLabel(const Vec3& pos, int nWidth, int nHeight, int nTexId, int nTexIconFlags, int srcOffsetX, int srcOffsetY, bool bDistanceScaleIcons, float fDistanceScale)
{
    const float fLabelDepthPrecision = 0.05f;
    Vec3 scrpos = view->WorldToView3D(pos);

    float fWidth = (float)nWidth;
    float fHeight = (float)nHeight;

    if (bDistanceScaleIcons)
    {
        float fScreenScale = view->GetScreenScaleFactor(pos);

        fWidth *= fDistanceScale / fScreenScale;
        fHeight *= fDistanceScale / fScreenScale;
    }

    STextureLabel tl;
    tl.x = scrpos.x + srcOffsetX;
    tl.y = scrpos.y + srcOffsetY;
    if (nTexIconFlags & TEXICON_ALIGN_BOTTOM)
    {
        tl.y -= fHeight / 2;
    }
    else if (nTexIconFlags & TEXICON_ALIGN_TOP)
    {
        tl.y += fHeight / 2;
    }
    tl.z = scrpos.z - (1.0f - scrpos.z) * fLabelDepthPrecision;
    tl.w = fWidth;
    tl.h = fHeight;
    tl.nTexId = nTexId;
    tl.flags = nTexIconFlags;
    tl.color[0] = m_color4b.r * (1.0f / 255.0f);
    tl.color[1] = m_color4b.g * (1.0f / 255.0f);
    tl.color[2] = m_color4b.b * (1.0f / 255.0f);
    tl.color[3] = m_color4b.a * (1.0f / 255.0f);

    // Try to not overflood memory with labels.
    if (m_textureLabels.size() < 100000)
    {
        m_textureLabels.push_back(tl);
    }
}

//////////////////////////////////////////////////////////////////////////
void DisplayContext::Flush2D()
{
#ifndef PHYSICS_EDITOR
    FUNCTION_PROFILER(GetIEditor()->GetSystem(), PROFILE_EDITOR);
#endif

    if (m_textureLabels.empty())
    {
        return;
    }

    int rcw, rch;
    view->GetDimensions(&rcw, &rch);

    TransformationMatrices backupSceneMatrices;

    renderer->Set2DMode(rcw, rch, backupSceneMatrices, 0.0f, 1.0f);

    renderer->SetState(GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA);
    //renderer->SetCullMode( R_CULL_NONE );

    float uvs[4], uvt[4];
    uvs[0] = 0;
    uvt[0] = 1;
    uvs[1] = 1;
    uvt[1] = 1;
    uvs[2] = 1;
    uvt[2] = 0;
    uvs[3] = 0;
    uvt[3] = 0;

    int nLabels = m_textureLabels.size();
    for (int i = 0; i < nLabels; i++)
    {
        STextureLabel& t = m_textureLabels[i];
        float w2 = t.w * 0.5f;
        float h2 = t.h * 0.5f;
        if (t.flags & TEXICON_ADDITIVE)
        {
            renderer->SetState(GS_BLSRC_ONE | GS_BLDST_ONE);
        }
        else if (t.flags & TEXICON_ON_TOP)
        {
            renderer->SetState(GS_NODEPTHTEST | GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA);
        }

        renderer->DrawImageWithUV(t.x - w2, t.y + h2, t.z, t.w, -t.h, t.nTexId, uvs, uvt, t.color[0], t.color[1], t.color[2], t.color[3]);

        if (t.flags & (TEXICON_ADDITIVE | TEXICON_ON_TOP)) // Restore state.
        {
            renderer->SetState(GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA);
        }
    }

    renderer->Unset2DMode(backupSceneMatrices);

    m_textureLabels.clear();
}
