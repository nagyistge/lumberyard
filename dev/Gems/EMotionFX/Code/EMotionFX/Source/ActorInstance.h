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

#pragma once

#include <AzCore/Math/Vector2.h>
// include MCore related files
#include "EMotionFXConfig.h"
#include <MCore/Source/Vector.h>
#include <MCore/Source/Quaternion.h>
#include <MCore/Source/Ray.h>
#include <MCore/Source/MultiThreadManager.h>
#include "BaseObject.h"
#include "Actor.h"
#include "Transform.h"
#include "AnimGraphPosePool.h"

#include <AzCore/RTTI/TypeInfo.h>

MCORE_FORWARD_DECLARE(AttributeSet);

namespace EMotionFX
{
    // forward declarations
    class MotionSystem;
    class Attachment;
    class LocalSpaceController;
    class GlobalSpaceController;
    class AnimGraphInstance;
    class EyeBlinker;
    class GlobalPose;
    class MorphSetupInstance;


    /**
     * The actor instance class.
     * An actor instance represents an animated character in your game.
     * Each actor instance is created from some Actor object, which contains all the hierarchy information and possibly also
     * the transformation and mesh information. Still, each actor instance can be controlled and animated individually.
     */
    class EMFX_API ActorInstance
        : public BaseObject
    {
        MCORE_MEMORYOBJECTCATEGORY(ActorInstance, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ACTORINSTANCES);
        friend class Attachment;

    public:
        /**
         * The bounding volume generation types.
         */
        enum EBoundsType
        {
            BOUNDS_NODE_BASED           = 0,    /**< Calculate the bounding volumes based on the globalspace node positions. */
            BOUNDS_MESH_BASED           = 1,    /**< Calculate the bounding volumes based on the globalspace vertex positions. */
            BOUNDS_COLLISIONMESH_BASED  = 2,    /**< Calculate the bounding volumes based on the globalspace collision mesh vertex positions. */
            BOUNDS_NODEOBB_BASED        = 3,    /**< Calculate the bounding volumes based on the oriented bounding boxes of the nodes. Uses all 8 corner points of the individual node OBB boxes. */
            BOUNDS_NODEOBBFAST_BASED    = 4,    /**< Calculate the bounding volumes based on the oriented bounding boxes of the nodes. Uses the min and max point of the individual node OBB boxes. This is less accurate but faster. */
            BOUNDS_STATIC_BASED         = 5     /**< Calculate the bounding volumes based on an approximate box, based on the mesh bounds, and move this box along with the actor instance position. */
        };

        static ActorInstance* Create(Actor* actor, uint32 threadIndex = 0);

        /**
         * Get a pointer to the actor from which this is an instance.
         * @result A pointer to the actor from which this is an instance.
         */
        Actor* GetActor() const;

        /**
         * Get the unique identification number for the actor instance.
         * @return The unique identification number.
         */
        MCORE_INLINE uint32 GetID() const                                       { return mID; }

        /**
         * Set the unique identification number for the actor instance.
         * @param[in] id The unique identification number.
         */
        void SetID(uint32 id);

        /**
         * Get the motion system of this actor instance.
         * The motion system handles all the motion management and blending. If you want to play a motion or figure out
         * what motions are currently active, you have to use the motion system class.
         * @result A pointer to the motion system.
         */
        MotionSystem* GetMotionSystem() const;

        /**
         * Set the current motion system to use.
         * On default a MotionLayerSystem is set.
         * @param newSystem The new motion system to use.
         * @param delCurrentFromMem If set to true, the currently set motion system will be deleted from memory, before setting the new motion system.
         */
        void SetMotionSystem(MotionSystem* newSystem, bool delCurrentFromMem = true);

        /**
         * Get the anim graph instance.
         * This can return nullptr, in which case the motion system as returned by GetMotionSystem() will be used.
         * @result The anim graph instance.
         */
        MCORE_INLINE AnimGraphInstance* GetAnimGraphInstance() const          { return mAnimGraphInstance; }

        /**
         * Set the anim graph instance.
         * This can be nullptr, in which case the motion system as returned by GetMotionSystem() will be used.
         * @param instance The anim graph instance.
         */
        void SetAnimGraphInstance(AnimGraphInstance* instance);

        /**
         * Get the transformation data class for this actor instance.
         * This transformation data class gives you access to all the transforms of the nodes in the actor.
         * So if you wish to get or set any transformations, you can do it with the object returned by this method.
         * @result A pointer to the transformation data object.
         */
        MCORE_INLINE TransformData* GetTransformData() const                    { return mTransformData; }

        /**
         * Enable or disable this actor instance.
         * Disabled actor instances are not processed at all. It will be like they do not exist.
         * On default the actor instance is enabled, after creation.
         * You can disable an actor instance that acts as skin attachment, but is not currently attached. This way the clothing items your character doesn't wear won't take processing power.
         * It is always faster not to have the actor instance created at all in such case though.
         * @param enabled Specifies whether this actor instance is enabled or not.
         */
        void SetIsEnabled(bool enabled);

        /**
         * Check whether this actor instance is enabled or not.
         * Disabled actor instances are not updated and processed.
         * @result Returns true when enabled, or false when disabled.
         */
        MCORE_INLINE bool GetIsEnabled() const                                  { return (mBoolFlags & BOOL_ENABLED) != 0; }

        /**
         * Check the visibility flag.
         * This flag has been set by the user and identifies if this actor instance is visible or not.
         * This is used internally by the schedulers, so that heavy calculations can be skipped on invisible characters.
         * @result Returns true when the actor instance is marked as visible, otherwise false is returned.
         */
        MCORE_INLINE bool GetIsVisible() const                                  { return (mBoolFlags & BOOL_ISVISIBLE) != 0; }

        /**
         * Change the visibility state.
         * @param isVisible Set to true when the actor instance is visible, otherwise set it to false.
         */
        void SetIsVisible(bool isVisible);

        /**
         * Specifies if this actor instance is visible or not.
         * This recursively propagates its visibility status to child attachments.
         * So if your horse is visibe then the rider that is attached on top of the horse will also become marked as visible.
         * @param Specify if all nodes touched should be marked as visible or not.
         */
        void RecursiveSetIsVisible(bool isVisible);


        /**
         * Recursively set the actor instance visibility flag, upwards in hierarchy, moving from an attachment up to the root actor instance.
         * @param Specify if all nodes touched should be marked as visible or not.
         */
        void RecursiveSetIsVisibleTowardsRoot(bool isVisible);

        //------------------------------------------------

        /**
         * Use the skeletal LOD flags from the nodes of the corresponding actor and pass them over to this actor instance.
         * The actor keeps the information about which nodes are enabled at what skeletal LOD and this is the transfer function to actually apply it to the actor instance
         * as each actor instance can be in a different skeletal LOD level.
         */
        void UpdateSkeletalLODFlags();

        /**
         * Calculate the number of disabled nodes for a given skeletal lod level.
         * @param[in] skeletalLODLevel The skeletal LOD level to calculate the number of disabled nodes for.
         * @return  The number of disabled nodes for the given skeletal LOD level.
         */
        uint32 CalcNumDisabledNodes(uint32 skeletalLODLevel) const;

        /**
         * Calculate the number of used skeletal LOD levels. Each actor instance alsways has 32 skeletal LOD levels while in most cases
         * not all of them are actually used. This function determines the number of skeletal LOD levels that actually disable some more nodes
         * relative to the previous LOD level.
         * @return The number of actually used skeletal LOD levels.
         */
        uint32 CalcNumSkeletalLODLevels() const;

        /**
         * Get the current used geometry and skeletal detail level.
         * In total there are 32 possible skeletal LOD levels, where 0 is the highest detail, and 31 the lowest detail.
         * Each detail level can disable specific nodes in the actor. These disabled nodes will not be processed at all by EMotion FX.
         * It is important to know that disabled nodes never should be used inside skinning information or on other places where their transformations
         * are needed.
         * @result The current LOD level.
         */
        uint32 GetLODLevel() const;

        /**
         * Set the current geometry and skeletal detail level, where 0 is the highest detail.
         * @param level The LOD level. Values higher than [GetNumGeometryLODLevels()-1] will be clamped to the maximum LOD.
         */
        void SetLODLevel(uint32 level);

        //--------------------------------

        /**
         * Set a pointer to some custom data you want to store and link with this actor instance object.
         * Custom data can for example link a game or engine object with this EMotion FX ActorInstance object.
         * An example is when EMotion FX triggers a motion event. You know the actor that triggered the event, but
         * you don't know directly what game or engine object is linked to this actor.
         * By using the custom data methods GetCustomData and SetCustomData you can set a pointer to your game or engine
         * object in each actor instance.
         * The pointer that you specify will not be deleted when the actor instance is being destructed.
         * @param customData A void pointer to the custom data to link with this actor instance.
         */
        void SetCustomData(void* customData, const AZ::Uuid& typeId);

        template<typename T>
        void SetCustomData(T* customData)
        {
            SetCustomData(customData, AZ::AzTypeInfo<T>::Uuid());
        }

        /**
         * Get a pointer to the custom data you stored and linked with this actor instance object.
         * Custom data can for example link a game or engine object with this EMotion FX ActorInstance object.
         * An example is when EMotion FX triggers a motion event. You know the actor that triggered the event, but
         * you don't know directly what game or engine object is linked to this actor.
         * By using the custom data methods GetCustomData and SetCustomData you can set a pointer to your game or engine
         * object in each actor instance.
         * The pointer that you specify will not be deleted when the actor instance is being destructed.
         * @result A void pointer to the custom data you have specified.
         */
        void* GetCustomData() const;

        /**
         * Typesafe version.
         * Use GetCustomData<T> to retrieve custom data in a type-safe manner. Null will be returned
         * if stored custom data is not of the requested type.
         */
        template <typename T>
        T* GetCustomData() const
        {
            if (mCustomDataType == AZ::AzTypeInfo<T>::Uuid())
            {
                return static_cast<T*>(mCustomData);
            }
            return nullptr;
        }

        /**
         * Retrieves the Uuid of the type currently stored in custom data.
         */
        const AZ::Uuid& GetCustomDataType() const
        {
            return mCustomDataType;
        }

        //-------------------------------------------------------------------------------------------

        // misc / partial update methods
        /**
         * Apply the morph targets transforms additively to the current local transforms as they are stored inside
         * the TransformData object that you retrieve with GetTransformData().
         * This will not apply any mesh morphs yet, as that is performed by the UpdateMeshDeformers() method.
         */
        void ApplyMorphSetup();

        // update the global transformation
        void UpdateGlobalTransform();

        /**
         * Update/construct the local space transformation matrices of all nodes.
         * This will update the data inside the TransformData class.
         * The local space matrices are constructed from the data stored in the local space transforms.
         */
        void UpdateLocalMatrices();

        /**
         * Update the global space matrices for all nodes.
         * This will update the data inside the TransformData class.
         * Forward kinematics using the local space matrices is performed to calculate the global space matrices.
         */
        void UpdateGlobalMatrices();

        /**
         * Update the global space matrices for all nodes, except root nodes.
         * This is used in case we are dealing with deformable attachments, where the root transforms have already been copied over from the
         * actor instance where this actor is attached to.
         */
        void UpdateGlobalMatricesForNonRoots();

        /**
         * Update the global space controllers.
         * Global space controllers will modify the global space transformation matrices of the nodes.
         * @param timePassedInSeconds The time passed in seconds, since the last update.
         */
        void UpdateGlobalSpaceControllers(float timePassedInSeconds);

        /**
         * If this is a skin attachment, it will update the global space matrices of this actor instance
         * by copying over the transforms from the actor it is attached to.
         * This is for example useful when you replace the upper body of a character, using a skin attachment.
         * You attach the upper body ot the main actor that contains the skeleton hierarchy that is being animated.
         * Now the node/bone transforms of the main actor will be copied over to this upper body attachment actor
         * so that it skins/deforms with the main actor's skeleton.
         */
        void UpdateMatricesIfSkinAttachment();

        /**
         * Update all the attachments.
         * This calls the update method for each attachment.
         */
        void UpdateAttachments();

        /**
         * Calculate the local space transformation matrix for a given node, and output the result in a given matrix.
         * This does not update the local space matrix of the node itself, but it just returns the matrix, how it would be
         * when it was currently being updated. It will use the local transform (pos/rot/scale) to calculate the matrix.
         * Also it will take into account the multiplication order as setup in the Actor object.
         * @param nodeIndex The node index/number to calculate the local space matrix for.
         * @param outMatrix A pointer to the matrix that will contain the local space matrix after executing this method.
         */
        void CalcLocalTM(uint32 nodeIndex, MCore::Matrix* outMatrix) const;

        /**
         * Calculate a local space matrix from a given pos/rot/scale/scalerot. And also take the Actor's multiplication order into account.
         * If you have the local transform components, like pos, rot, scale and scale rotation for a given node, and you wish to
         * construct a local space matrix from these components, for a given node in a given actor, then you should use this method.
         * Please note that this method does not update or modify the local space matrix that is stored inside the TransformData class of this
         * actor instance.
         * @param nodeIndex The node to calculate the local space matrix for.
         * @param scaleRot The scale rotation, which defines the space in which scaling has to happen.
         * @param rot The rotation of the node.
         * @param pos The translation.
         & @param scale The scale factor for each axis.
         * @param outMatrix A pointer to the matrix that will contain the local space matrix after executing this method.
         */
        void CalcLocalTM(uint32 nodeIndex, const MCore::Vector3& pos, const MCore::Quaternion& rot, const MCore::Vector3& scale, MCore::Matrix* outMatrix) const;

        //-------------------------------------------------------------------------------------------

        // main methods
        /**
         * Update the transformations of this actor instance.
         * This will calculate and update all the local transforms, local space matrices and global space matrices that
         * are stored inside the TransformData object of this actor instance.
         * This automatically updates all motion timers as well.
         * @param timePassedInSeconds The time passed in seconds, since the last frame or update.
         * @param updateMatrices When set to true the node matrices will all be updated.
         * @param sampleMotions When set to true motions will be sampled, or whole anim graphs if using those. When updateMatrices is set to false, motions will never be sampled, even if set to true.
         */
        void UpdateTransformations(float timePassedInSeconds, bool updateMatrices = true, bool sampleMotions = true);

        /**
         * Update/Process the mesh deformers.
         * This will apply skinning and morphing deformations to the meshes used by the actor instance.
         * All deformations happen on the CPU. So if you use pure GPU processing, you should not be calling this method.
         * Also invisible actor instances should not update their mesh deformers, as this can be very CPU intensive.
         * @param timePassedInSeconds The time passed in seconds, since the last frame or update.
         */
        void UpdateMeshDeformers(float timePassedInSeconds);

        //-------------------------------------------------------------------------------------------

        // bounding volume
        /**
         * Setup the automatic update settings of the bounding volume.
         * This allows you to specify at what time interval the bounding volume of the actor instance should be updated and
         * if this update should base its calculations on the nodes, mesh vertices or collision mesh vertices.
         * @param updateFrequencyInSeconds The bounds will be updated every "updateFrequencyInSeconds" seconds. A value of 0 would mean every frame and a value of 0.1 would mean 10 times per second.
         * @param boundsType The type of bounds calculation. This can be either based on the node's global space positions, the mesh global space positions or the collision mesh global space positions.
         * @param itemFrequency A value of 1 would mean every node or vertex will be taken into account in the bounds calculation.
         *                      A value of 2 would mean every second node or vertex would be taken into account. A value of 5 means every 5th node or vertex, etc.
         *                      Higher values will result in faster bounds updates, but also in possibly less accurate bounds. This value must 1 or higher. Zero is not allowed.
         */
        void SetupAutoBoundsUpdate(float updateFrequencyInSeconds, EBoundsType boundsType = BOUNDS_NODE_BASED, uint32 itemFrequency = 1);

        /**
         * Check if the automatic bounds update feature is enabled.
         * It will use the settings as provided to the SetupAutoBoundsUpdate method.
         * @result Returns true when the automatic bounds update feature is enabled, or false when it is disabled.
         */
        bool GetBoundsUpdateEnabled() const;

        /**
         * Get the automatic bounds update time frequency.
         * This specifies the time interval between bounds updates. A value of 0.1 would mean that the bounding volumes
         * will be updated 10 times per second. A value of 0.25 means 4 times per second, etc. A value of 0 can be used to
         * force the updates to happen every frame.
         * @result Returns the automatic bounds update frequency time interval, in seconds.
         */
        float GetBoundsUpdateFrequency() const;

        /**
         * Get the time passed since the last automatic bounds update.
         * @result The time passed, in seconds, since the last automatic bounds update.
         */
        float GetBoundsUpdatePassedTime() const;

        /**
         * Get the bounding volume auto-update type.
         * This can be either based on the node's global space positions, the mesh vertex global space positions, or the
         * collision mesh vertex global space postitions.
         * @result The bounding volume update type.
         */
        EBoundsType GetBoundsUpdateType() const;

        /**
         * Get the bounding volume auto-update item frequency.
         * A value of 1 would mean every node or vertex will be taken into account in the bounds calculation.
         * A value of 2 would mean every second node or vertex would be taken into account. A value of 5 means every 5th node or vertex, etc.
         * Higher values will result in faster bounds updates, but also in possibly less accurate bounds.
         * The value returned will always be equal or greater than one.
         * @result The item frequency, which will be equal or greater than one.
         */
        uint32 GetBoundsUpdateItemFrequency() const;

        /**
         * Set the auto-bounds update time frequency, in seconds.
         * This specifies the time interval between bounds updates. A value of 0.1 would mean that the bounding volumes
         * will be updated 10 times per second. A value of 0.25 means 4 times per second, etc. A value of 0 can be used to
         * force the updates to happen every frame.
         * @param seconds The amount of seconds between every bounding volume update.
         */
        void SetBoundsUpdateFrequency(float seconds);

        /**
         * Set the time passed since the last automatic bounds update.
         * @param seconds The amount of seconds, since the last automatic update.
         */
        void SetBoundsUpdatePassedTime(float seconds);

        /**
         * Set the bounding volume auto-update type.
         * This can be either based on the node's global space positions, the mesh vertex global space positions, or the
         * collision mesh vertex global space postitions.
         * @param bType The bounding volume update type.
         */
        void SetBoundsUpdateType(EBoundsType bType);

        /**
         * Set the bounding volume auto-update item frequency.
         * A value of 1 would mean every node or vertex will be taken into account in the bounds calculation.
         * A value of 2 would mean every second node or vertex would be taken into account. A value of 5 means every 5th node or vertex, etc.
         * Higher values will result in faster bounds updates, but also in possibly less accurate bounds.
         * The frequency value must always be greater than or equal to one.
         * @param freq The item frequency, which has to be 1 or above. Higher values result in faster performance, but lower accuracy.
         */
        void SetBoundsUpdateItemFrequency(uint32 freq);

        /**
         * Specify whether you want the auto-bounds update to be enabled or disabled.
         * On default, after creation of the actor instance it is enabled, using node based, with an update frequency of 1 and
         * an update frequency of 0.1 (ten times per second).
         * @param enable Set to true when you want to enable the bounds automatic update feature, or false when you'd like to disable it.
         */
        void SetBoundsUpdateEnabled(bool enable);

        /**
         * Update the bounding volumes of the actor.
         * @param geomLODLevel The geometry level of detail number to generate the LOD for. Must be in range of 0..GetNumGeometryLODLevels()-1.
         *                     The same LOD level will be chosen for the attachments when they will be included.
         *                     If the specified LOD level is lower than the number of attachment LODS, the lowest attachment LOD will be chosen.
         * @param boundsType The bounding volume generation method, each having different accuracies and generation speeds.
         * @param itemFrequency Depending on the type of bounds you specify to the boundsType parameter, this specifies how many "vertices or nodes" the
         *                      updating functions should skip. If you specified a value of 4, and use BOUND_MESH_BASED or collision mesh based, every
         *                      4th vertex will be included in the bounds calculation, so only processing 25% of the total number of vertices. The same goes for
         *                      node based bounds, but then it will process every 4th node. Of course higher values produce less accurate results, but are faster to process.
         */
        void UpdateBounds(uint32 geomLODLevel, EBoundsType boundsType = BOUNDS_NODE_BASED, uint32 itemFrequency = 1);

        /**
         * Update the base static axis aligned bounding box shape.
         * This is a quite heavy function in comparision to the CalcStaticBasedAABB. Basically the dimensions of the static based aabb are calculated here.
         * First it will try to generate the aabb from the meshes. If there are no meshes it will use a node based aabb as basis.
         * After that it will find the maximum of the depth, width and height, and makes all dimensions the same as this max value.
         * This function is generally only executed once, when creating the actor instance.
         * The CalcStaticBasedAABB function then simply translates this box along with the actor instance's position.
         */
        void UpdateStaticBasedAABBDimensions();

        void SetStaticBasedAABB(const MCore::AABB& aabb);
        void GetStaticBasedAABB(MCore::AABB* outAABB);
        const MCore::AABB& GetStaticBasedAABB() const;

        /**
         * Calculate an axis aligned bounding box that can be used as static AABB. It is static in the way that the volume does not change. It can however be translated as it will move
         * with the character's position. The box will contain the mesh based bounds, and finds the maximum of the width, depth and height, and makes all depth width and height equal to this value.
         * So the box will in most cases be too wide, but this is done on purpose. This can be used when the character is outside of the frustum, but we still update the position.
         * We can then use this box to test for visibility again.
         * If there are no meshes present, a widened node based box will be used instead as basis.
         * @param outResult The resulting bounding box, moved along with the actor instance's position.
         */
        void CalcStaticBasedAABB(MCore::AABB* outResult);

        /**
         * Calculate the axis aligned bounding box based on the globalspace positions of the nodes.
         * @param outResult The AABB where this method should store the resulting box in.
         * @param nodeFrequency This will include every "nodeFrequency"-th node. So a value of 1 will include all nodes. A value of 2 would
         *                      process every second node, meaning that half of the nodes will be skipped. A value of 4 would process every 4th node, etc.
         */
        void CalcNodeBasedAABB(MCore::AABB* outResult, uint32 nodeFrequency = 1);

        /**
         * Calculate the axis aligned bounding box based on the globalspace vertex coordinates of the meshes.
         * If the actor has no meshes, the created box will be invalid.
         * @param geomLODLevel The geometry LOD level to calculate the box for.
         * @param outResult The AABB where this method should store the resulting box in.
         * @param vertexFrequency This includes every "vertexFrequency"-th vertex. So for example a value of 2 would skip every second vertex and
         *                        so will process half of the vertices. A value of 4 would process only each 4th vertex, etc.
         */
        void CalcMeshBasedAABB(uint32 geomLODLevel, MCore::AABB* outResult, uint32 vertexFrequency = 1);

        /**
         * Calculate the axis aligned bounding box based on the globalspace vertex coordinates of the collision meshes.
         * If the actor has no collision meshes, the created box will be invalid.
         * @param geomLODLevel The geometry LOD level to calculate the box for.
         * @param outResult The AABB where this method should store the resulting box in.
         * @param vertexFrequency This includes every "vertexFrequency"-th vertex. So for example a value of 2 would skip every second vertex and
         *                        so will process half of the vertices. A value of 4 would process only each 4th vertex, etc.
         */
        void CalcCollisionMeshBasedAABB(uint32 geomLODLevel, MCore::AABB* outResult, uint32 vertexFrequency = 1);

        /**
         * Calculate the axis aligned bounding box that contains the object oriented boxes of all nodes.
         * The OBB (oriented bounding box) of each node is calculated by fitting an OBB to its mesh.
         * The OBB of nodes that act as bones and have no meshes themselves are fit to the set of vertices that are influenced by the given bone.
         * This method will give more accurate results than the CalcNodeBasedAABB method in trade for a bit lower performance.
         * Also one big advantage of this method is that you can use these bounds for hit detection, without having artists setup collision meshes.
         * @param outResult The AABB where this method should store the resulting box in.
         * @param nodeFrequency This will include every "nodeFrequency"-th node. So a value of 1 will include all nodes. A value of 2 would
         *                      process every second node, meaning that half of the nodes will be skipped. A value of 4 would process every 4th node, etc.
         */
        void CalcNodeOBBBasedAABB(MCore::AABB* outResult, uint32 nodeFrequency = 1);

        /**
         * Calculate the axis aligned bounding box that contains the object oriented boxes of all nodes.
         * The OBB (oriented bounding box) of each node is calculated by fitting an OBB to its mesh.
         * The OBB of nodes that act as bones and have no meshes themselves are fit to the set of vertices that are influenced by the given bone.
         * This method will give more accurate results than the CalcNodeBasedAABB method in trade for a bit lower performance.
         * Also one big advantage of this method is that you can use these bounds for hit detection, without having artists setup collision meshes.
         * NOTE: this is a faster variant from the CalcNodeOBBBasedAABB method. The difference is that this method only transforms the min and max point of the box in local space.
         * Therefore it is less accurate, but it might still be enough. The original CalcNodeOBBBasedAABB method calculates the 8 corner points of the node obb boxes.
         * @param outResult The AABB where this method should store the resulting box in.
         * @param nodeFrequency This will include every "nodeFrequency"-th node. So a value of 1 will include all nodes. A value of 2 would
         *                      process every second node, meaning that half of the nodes will be skipped. A value of 4 would process every 4th node, etc.
         */
        void CalcNodeOBBBasedAABBFast(MCore::AABB* outResult, uint32 nodeFrequency = 1);

        /**
         * Get the axis aligned bounding box.
         * This box will be updated once the UpdateBounds method is called.
         * That method is also called automatically when the bounds auto-update feature is enabled.
         * @result The axis aligned bounding box.
         */
        const MCore::AABB& GetAABB() const;

        /**
         * Set the axis aligned bounding box.
         * Please beware that this box will get automatically overwritten when automatic bounds update is enabled.
         * @param aabb The axis aligned bounding box to store.
         */
        void SetAABB(const MCore::AABB& aabb);

        //-------------------------------------------------------------------------------------------

        // local space controllers
        /**
         * Add a local space controller.
         * Please keep in mind that you have to activate your controller before it will have any influence.
         * Activating your controller can be done using the LocalSpaceController::Activate() method.
         * Also note that you don't have to delete the controller yourself, as it will be deleted automatically from memory
         * when deleting the actor instance.
         * The order in which you add the local space controllers is not important. You can control the priority level of the
         * controller when using the LocalSpaceController::Activate() method. A controller is basically just a motion, and the motion
         * priorities can be used to define what motions overwrite others.
         * @param controller The local space controller to add.
         */
        void AddLocalSpaceController(LocalSpaceController* controller);

        /**
         * Get the number of local space controllers that have been added.
         * @result The number of local space controllers.
         */
        uint32 GetNumLocalSpaceControllers() const;

        /**
         * Get a local space controller that has been added to this actor before.
         * @param nr The controller number, which must be in range of [0..GetNumLocalSpaceControllers()-1].
         * @result A pointer to the controller.
         */
        LocalSpaceController* GetLocalSpaceController(uint32 nr) const;

        /**
         * Remove a given local space controller that has been added before.
         * @param nr The controller number, which must be in range of [0..GetNumLocalSpaceControllers()-1].
         * @param delFromMem Set this to true when you want the controller that gets removed also to be deleted from memory.
         *                   When you set this to false, it will not be deleted from memory, but only removed from the array of controllers
         *                   that is stored locally inside this actor instance.
         */
        void RemoveLocalSpaceController(uint32 nr, bool delFromMem = true);

        /**
         * Remove a given local space controller that has been added before.
         * The specified controller has to be part of this actor instance, otherwise an assert will occur.
         * @param controller The pointer to the controller to remove.
         * @param delFromMem Set this to true when you want the controller that gets removed also to be deleted from memory.
         *                   When you set this to false, it will not be deleted from memory, but only removed from the array of controllers
         *                   that is stored locally inside this actor instance.
         */
        void RemoveLocalSpaceController(LocalSpaceController* controller, bool delFromMem = true);

        /**
         * Remove all local space controllers that have been added to this actor instance.
         * @param delFromMem Set this to true when you want the controller that gets removed also to be deleted from memory.
         *                   When you set this to false, it will not be deleted from memory, but only removed from the array of controllers
         *                   that is stored locally inside this actor instance.
         */
        void RemoveAllLocalSpaceControllers(bool delFromMem = true);

        //-------------------------------------------------------------------------------------------

        // global space controllers
        /**
         * Add a given global space controller to this actor instance.
         * The order in which you add global space controllers is important!
         * They will be executed in the order you add them. Where the first one you add will get executed first.
         * This method will add it to the back of the list, so that it will be executed as last.
         * If you need to control the position in the list where it should insert the controller, use the
         * InsertGlobalSpaceController method instead.
         * @param controller The global space controller to add to the back of the controller list.
         */
        void AddGlobalSpaceController(GlobalSpaceController* controller);

        /**
         * Insert a global space controller inside the list of global space controllers.
         * This will allow you to control in what order the controllers have to be executed.
         * @param insertIndex The insert location, which must be in range of [0..GetNumGlobalSpaceControllers()-1].
         * @param controller The global space controller to insert.
         */
        void InsertGlobalSpaceController(uint32 insertIndex, GlobalSpaceController* controller);

        /**
         * Get the number of global space controllers inside this actor instance.
         * @result The number of global space controllers.
         */
        uint32 GetNumGlobalSpaceControllers() const;

        /**
         * Get a given global space controller.
         * @param nr The global space controller number/index, which must be in range of [0..GetNumGlobalSpaceControllers()-1].
         * @result A pointer to the global space controller.
         */
        GlobalSpaceController* GetGlobalSpaceController(uint32 nr) const;

        /**
         * Has a global space controller of the given type.
         * @param typeID The global space controller type to check for.
         * @result True in case there is a global space controller of the given type, false if not.
         */
        bool GetHasGlobalSpaceController(uint32 typeID) const;

        /**
         * Find the first global space controller of the given type.
         * @param typeID The global space controller type to search for.
         * @result A pointer to the global space controller of the given type.
         */
        GlobalSpaceController* FindGlobalSpaceControllerByType(uint32 typeID) const;

        /**
         * Find the index of the first global space controller of the given type.
         * @param typeID The global space controller type to search for.
         * @result The index of the global space controller with the given type.
         */
        uint32 FindGlobalSpaceControllerIndexByType(uint32 typeID) const;

        /**
         * Remove a given global space controller.
         * @param nr The controller number, which must be in range of [0..GetNumGlobalSpaceControllers()-1].
         * @param delFromMem Set this to true when you want the controller that gets removed also to be deleted from memory.
         *                   When you set this to false, it will not be deleted from memory, but only removed from the array of controllers
         *                   that is stored locally inside this actor instance.
         */
        void RemoveGlobalSpaceController(uint32 nr, bool delFromMem = true);

        /**
         * Remove a given global space controller that has been added before.
         * The specified controller has to be part of this actor instance, otherwise an assert will occur.
         * @param controller The pointer to the controller to remove.
         * @param delFromMem Set this to true when you want the controller that gets removed also to be deleted from memory.
         *                   When you set this to false, it will not be deleted from memory, but only removed from the array of controllers
         *                   that is stored locally inside this actor instance.
         */
        void RemoveGlobalSpaceController(GlobalSpaceController* controller, bool delFromMem = true);

        /**
         * Remove all global space controllers that have been added to this actor instance.
         * @param delFromMem Set this to true when you want the controller that gets removed also to be deleted from memory.
         *                   When you set this to false, it will not be deleted from memory, but only removed from the array of controllers
         *                   that is stored locally inside this actor instance.
         */
        void RemoveAllGlobalSpaceControllers(bool delFromMem = true);

        //-------------------------------------------------------------------------------------------

        /**
         * Set the local space position of this actor instance.
         * This is relative to its parent (if it is attached ot something). Otherwise it is in global space.
         * @param position The position/translation to use.
         */
        MCORE_INLINE void SetLocalPosition(const MCore::Vector3& position)          { mLocalTransform.mPosition = position; }

        /**
         * Set the local rotation of this actor instance.
         * This is relative to its parent (if it is attached ot something). Otherwise it is in global space.
         * @param rotation The rotation to use.
         */
        MCORE_INLINE void SetLocalRotation(const MCore::Quaternion& rotation)       { mLocalTransform.mRotation = rotation; }

        EMFX_SCALECODE
        (
            /**
             * Set the local scale of this actor instance.
             * This is relative to its parent (if it is attached ot something). Otherwise it is in global space.
             * @param scale The scale to use.
             */
            MCORE_INLINE void SetLocalScale(const MCore::Vector3 & scale)            {
                mLocalTransform.mScale = scale;
            }

            /**
             * Get the local space scale.
             * This is relative to its parent (if it is attached ot something). Otherwise it is in global space.
             * @result The local space scale factor for each axis.
             */
            MCORE_INLINE const MCore::Vector3& GetLocalScale() const                { return mLocalTransform.mScale;
            }
        )

        /**
         * Get the local space position/translation of this actor instance.
         * This is relative to its parent (if it is attached ot something). Otherwise it is in global space.
         * @result The local space position.
         */
        MCORE_INLINE const MCore::Vector3 & GetLocalPosition() const                 {
            return mLocalTransform.mPosition;
        }

        /**
         * Get the local space rotation of this actor instance.
         * This is relative to its parent (if it is attached ot something). Otherwise it is in global space.
         * @result The local space rotation.
         */
        MCORE_INLINE const MCore::Quaternion& GetLocalRotation() const              { return mLocalTransform.mRotation; }


        MCORE_INLINE const MCore::Vector3& GetGlobalPosition() const                { return mGlobalTransform.mPosition;    }
        MCORE_INLINE const MCore::Quaternion& GetGlobalRotation() const             { return mGlobalTransform.mRotation;    }
        EMFX_SCALECODE
        (
            MCORE_INLINE const MCore::Vector3 & GetGlobalScale() const               { return mGlobalTransform.mScale;
            }
        )

        MCORE_INLINE void SetLocalTransform(const Transform& transform)             { mLocalTransform = transform; }

        MCORE_INLINE const Transform& GetLocalTransform() const                     { return mLocalTransform; }
        MCORE_INLINE const Transform& GetGlobalTransform() const                    { return mGlobalTransform; }

        //-------------------------------------------------------------------------------------------

        // attachments
        /**
         * Add an attachment to this actor.
         * Please note that each attachment can only belong to one actor instance.
         * @param attachment The attachment to add.
         */
        void AddAttachment(Attachment* attachment);

        /**
         * Remove a given attachment.
         * @param nr The attachment number, which must be in range of [0..GetNumAttachments()-1].
         * @param delFromMem Set this to true when you want the attachment that gets removed also to be deleted from memory.
         *                   When you set this to false, it will not be deleted from memory, but only removed from the array of attachments
         *                   that is stored locally inside this actor instance.
         */
        void RemoveAttachment(uint32 nr, bool delFromMem = true);

        /**
         * Remove all attachments from this actor instance.
         * @param delFromMem Set this to true when you want the attachments also to be deleted from memory.
         *                   When you set this to false, it will not be deleted from memory, but only removed from the array of attachments
         *                   that is stored locally inside this actor instance.
         */
        void RemoveAllAttachments(bool delFromMem = true);

        /**
         * Remove an attachment that uses a specified actor instance.
         * @param actorInstance The actor instance that the attachment is using.
         * @param delFromMem Set this to true when you want the attachment also to be deleted from memory.
         *                   When you set this to false, it will not be deleted from memory, but only removed from the array of attachments
         *                   that is stored locally inside this actor instance.
         */
        bool RemoveAttachment(ActorInstance* actorInstance, bool delFromMem = true);

        /**
         * Find the attachment number that uses a given actor instance.
         * @param actorInstance The actor instance that the attachment you search for is using.
         * @result Returns the attachment number, in range of [0..GetNumAttachments()-1], or MCORE_INVALIDINDEX32 when no attachment
         *         using the specified actor instance can be found.
         */
        uint32 FindAttachmentNr(ActorInstance* actorInstance);

        /**
         * Get the number of attachments that have been added to this actor instance.
         * @result The number of attachments added to this actor instance.
         */
        uint32 GetNumAttachments() const;

        /**
         * Get a specific attachment.
         * @param nr The attachment number, which must be in range of [0..GetNumAttachments()-1].
         * @result A pointer to the attachment.
         */
        Attachment* GetAttachment(uint32 nr) const;

        /**
         * Check whether this actor instance also is an attachment or not.
         * So if this actor instance is like a cowboy, and the cowboy is attached to a horse, then this will return true.
         * If the cowboy (so this actor instance) would not be attached to anything, it will return false.
         * @result Returns true when this actor instance is also an attachment to some other actor instance. Otherwise false is returned.
         */
        bool GetIsAttachment() const;

        /**
         * Get the actor instance where this actor instance is attached to.
         * This will return nullptr in case this actor instance is not an attachment.
         * @result A pointer to the actor instance where this actor instance is attached to.
         */
        ActorInstance* GetAttachedTo() const;

        /**
         * Find the root actor instance.
         * If this actor instance object represents a gun, which is attached to a cowboy, which is attached to a horse, then
         * the attachment root that is returned by this method is the horse.
         * @result The attachment root, or a pointer to itself when this actor instance isn't attached to anything.
         */
        ActorInstance* FindAttachmentRoot() const;

        /**
         * Get the attachment where this actor instance is part of.
         * So if this actor instance is a gun, and the gun is attached to some cowboy actor instance, then the Attachment object that
         * is returned here, is the attachment object for the gun that was added to the cowboy actor instance.
         * @result The attachment where this actor instance takes part of, or nullptr when this actor instance isn't an attachment.
         */
        Attachment* GetSelfAttachment() const;

        /**
         * Check if the actor instance is a skin attachment.
         * @result Returns true when the actor instance itself is a skin attachment, otherwise false is returned.
         */
        bool GetIsSkinAttachment() const;

        //-------------------------------------------------------------------------------------------

        /**
         * Update all dependencies of this actor instance.
         */
        void UpdateDependencies();

        /**
         * Recursively add dependencies for the given actor.
         * This will add all dependencies stored in the specified actor, to this actor instance.
         * Also it will recurse into the dependencies of the dependencies of the given actor.
         * @param actor The actor we should recursively copy the dependencies from.
         */
        void RecursiveAddDependencies(Actor* actor);

        /**
         * Get the number of dependencies that this actor instance has on other actors.
         * @result The number of dependencies.
         */
        uint32 GetNumDependencies() const;

        /**
         * Get a given dependency.
         * @param nr The dependency number to get, which must be in range of [0..GetNumDependencies()].
         * @result A pointer to the dependency.
         */
        Actor::Dependency* GetDependency(uint32 nr);

        //-------------------------------------------------------------------------------------------

        /**
         * Calculate the global space scale values by applying forward kinematics on the local space scale values.
         * This will output a scale value for each node in the actor where this is an instance from.
         * @param outScales The array to store the scale values in. The size of this array must be at least the value returned by GetNumNodes().
         */
        void CalcGlobalScales(MCore::Vector3* outScales) const;

        /**
         * This will calculate the global space scale values and return them.
         * No calculation is done when they already have been calculated.
         * You should not free the pointer that is returned.
         * @result A pointer to the global space scale values.
         */
        MCore::Vector3* CalcGlobalScales();

        // clone data from another actor instance
        /**
         * Clone all local space controllers that have been added to another actor instance, and add them to this actor instance.
         * Already existing controllers inside this actor instance will remain and will not be removed.
         * @param sourceActor The source actor, where to copy over the controllers from.
         * @param neverActivate When set to true, the cloned controllers will not be activated in case they are active inside the specified sourceActor.
         *                      When set to false, the cloned controllers will also be activated when the they are currently active in the sourceActor.
         */
        void CloneLocalSpaceControllers(ActorInstance* sourceActor, bool neverActivate = false);

        /**
         * Clone all global space controllers that have been added to another actor instance, and add them to this actor instance.
         * Already existing controllers inside this actor instance will remain and will not be removed.
         * @param sourceActor The source actor, where to copy over the controllers from.
         */
        void CloneGlobalSpaceControllers(ActorInstance* sourceActor);

        /**
         * Recursively update the global space matrix.
         * This will update the global space matrix of a given node, and go down the hierarchy to update all child nodes as well.
         * @param nodeIndex The node to start updating the global space matrix for.
         * @param globalTM The value of the global space matrix to set. When this is nullptr, it will multiply its current local space matrix with the
         *                global space matrix of its parent node to use as global space matrix.
         * @param outGlobalMatrixArray The array to output the global space matrices in. This array has to contain at least GetNumNodes() matrices.
         */
        void RecursiveUpdateGlobalTM(uint32 nodeIndex, const MCore::Matrix* globalTM = nullptr, MCore::Matrix* outGlobalMatrixArray = nullptr);

        /**
         * Get the morph setup instance.
         * This doesn't contain the actual morph targets, but the unique settings per morph target.
         * These unique settings are the weight value per morph target, and a boolean that specifies if the morph target
         * weight should be controlled automatically (extracted from motions) or manually by user specified values.
         * If you want to access the actual morph targets, you have to use the Actor::GetMorphSetup() method.
         * When the morph setup instance object doesn't contain any morph targets, no morphing is used.
         * @result A pointer to the morph setup instance object for this actor instance.
         */
        MorphSetupInstance* GetMorphSetupInstance() const;

        //-----------------------------------------------------------------

        /**
         * Check for an intersection between the collision mesh of this actor and a given ray.
         * Returns a pointer to the node it detected a collision in case there is a collision with any of the collision meshes of all nodes of this actor.
         * If there is no collision mesh attached to the nodes, no intersection test will be done, and nullptr will be returned.
         * @param lodLevel The level of detail to check the collision with.
         * @param ray The ray to check.
         * @return A pointer to the node we detected the first intersection with (doesn't have to be the closest), or nullptr when no intersection found.
         */
        Node* IntersectsCollisionMesh(uint32 lodLevel, const MCore::Ray& ray) const;

        /**
         * Check for an intersection between the collision mesh of this actor and a given ray, and calculate the closest intersection point.
         * If there is no collision mesh attached to the nodes, no intersection test will be done, and nullptr will be returned.
         * Returns a pointer to the node it detected a collision in case there is a collision with the collision meshes of the actor, 'outIntersect'
         * will contain the closest intersection point in case there is a collision. Use the other Intersects method when you do not need the intersection
         * point (since that one is faster).
         * @param lodLevel The level of detail to check the collision with.
         * @param ray The ray to test with.
         * @param outIntersect A pointer to the vector to store the intersection point in, in case of a collision (nullptr allowed).
         * @param outNormal A pointer to the vector to store the normal at the intersection point, in case of a collision (nullptr allowed).
         * @param outUV A pointer to the vector to store the uv coordinate at the intersection point (nullptr allowed).
         * @param outBaryU A pointer to a float in which the method will store the barycentric U coordinate, to be used to interpolate values on the triangle (nullptr allowed).
         * @param outBaryV A pointer to a float in which the method will store the barycentric V coordinate, to be used to interpolate values on the triangle (nullptr allowed).
         * @param outIndices A pointer to an array of 3 integers, which will contain the 3 vertex indices of the closest intersecting triangle. Even on polygon meshes with polygons of more than 3 vertices three indices are returned. In that case the indices represent a sub-triangle inside the polygon.
         *                   A value of nullptr is allowed, which will skip storing the resulting triangle indices.
         * @return A pointer to the node we detected the closest intersection with, or nullptr when no intersection found.
         */
        Node* IntersectsCollisionMesh(uint32 lodLevel, const MCore::Ray& ray, MCore::Vector3* outIntersect, MCore::Vector3* outNormal = nullptr, AZ::Vector2* outUV = nullptr, float* outBaryU = nullptr, float* outBaryV = nullptr, uint32* outIndices = nullptr) const;

        /**
         * Check for an intersection between the real mesh (if present) of this actor and a given ray.
         * Returns a pointer to the node it detected a collision in case there is a collision with any of the real meshes of all nodes of this actor.
         * If there is no mesh attached to this node, no intersection test will be performed and nullptr will be returned.
         * @param lodLevel The level of detail to check the collision with.
         * @param ray The ray to test with.
         * @return Returns a pointer to itself when an intersection occurred, or nullptr when no intersection found.
         */
        Node* IntersectsMesh(uint32 lodLevel, const MCore::Ray& ray) const;

        /**
         * Checks for an intersection between the real mesh (if present) of this actor and a given ray.
         * Returns a pointer to the node it detected a collision in case there is a collision with any of the real meshes of all nodes of this actor,
         * 'outIntersect' will contain the closest intersection point in case there is a collision.
         * Both the intersection point and normal which are returned are in global space.
         * Use the other Intersects method when you do not need the intersection point (since that one is faster).
         * Both the intersection point and normal which are returned are in global space.
         * @param lodLevel The level of detail to check the collision with.
         * @param ray The ray to test with.
         * @param outIntersect A pointer to the vector to store the intersection point in, in case of a collision (nullptr allowed).
         * @param outNormal A pointer to the vector to store the normal at the intersection point, in case of a collision (nullptr allowed).
         * @param outUV A pointer to the vector to store the uv coordinate at the intersection point (nullptr allowed).
         * @param outBaryU A pointer to a float in which the method will store the barycentric U coordinate, to be used to interpolate values on the triangle (nullptr allowed).
         * @param outBaryV A pointer to a float in which the method will store the barycentric V coordinate, to be used to interpolate values on the triangle (nullptr allowed).
         * @param outIndices A pointer to an array of 3 integers, which will contain the 3 vertex indices of the closest intersecting triangle. Even on polygon meshes with polygons of more than 3 vertices three indices are returned. In that case the indices represent a sub-triangle inside the polygon.
         *                   A value of nullptr is allowed, which will skip storing the resulting triangle indices.
         * @return A pointer to the node we detected the closest intersection with, or nullptr when no intersection found.
         */
        Node* IntersectsMesh(uint32 lodLevel, const MCore::Ray& ray, MCore::Vector3* outIntersect, MCore::Vector3* outNormal = nullptr, AZ::Vector2* outUV = nullptr, float* outBaryU = nullptr, float* outBaryV = nullptr, uint32* outStartIndex = nullptr) const;

        void SetParentGlobalTransform(const Transform& transform);
        const Transform& GetParentGlobalTransform() const;

        /**
         * Set whether calls to ActorUpdateCallBack::OnRender() for this actor instance should be made or not.
         * @param enabled Set to true when the ActorUpdateCallBack::OnRender() should be called for this actor instance, otherwise set to false.
         */
        void SetRender(bool enabled);

        /**
         * Check if calls to ActorUpdateCallBack::OnRender() for this actor instance are being made or not.
         * @result Returns true when the ActorUpdateCallBack::OnRender() is being called for this actor instance. False is returned in case this callback won't be executed for this actor instance.
         */
        bool GetRender() const;

        void SetIsUsedForVisualization(bool enabled);
        bool GetIsUsedForVisualization() const;

        /**
         * Marks the actor instance as used by the engine runtime, as opposed to the tool suite.
         */
        void SetIsOwnedByRuntime(bool isOwnedByRuntime);
        bool GetIsOwnedByRuntime() const;

        //-------------

        /**
         * Enable a specific node.
         * This will activate motion sampling, transformation and blending calculations for the given node.
         * @param nodeIndex The node number to enable. This must be in range of [0..Actor::GetNumNodes()-1].
         */
        void EnableNode(uint16 nodeIndex);

        /**
         * Disable a specific node.
         * This will disable motion sampling, transformation and blending calculations for the given node.
         * @param nodeIndex The node number to disable. This must be in range of [0..Actor::GetNumNodes()-1].
         */
        void DisableNode(uint16 nodeIndex);

        /**
         * Get direct access to the array of enabled nodes.
         * @result A read only reference to the array of enabled nodes. The values inside of this array are the node numbers of the enabled nodes.
         */
        MCORE_INLINE const MCore::Array<uint16>& GetEnabledNodes() const        { return mEnabledNodes; }

        /**
         * Get the number of enabled nodes inside this actor instance.
         * @result The number of nodes that have been enabled and are being updated.
         */
        MCORE_INLINE uint32 GetNumEnabledNodes() const                          { return mEnabledNodes.GetLength(); }

        /**
         * Get the node number of a given enabled node.
         * @param index An index in the array of enabled nodes. This must be in range of [0..GetNumEnabledNodes()-1].
         * @result The node number, which relates to Actor::GetNode( returnValue ).
         */
        MCORE_INLINE uint16 GetEnabledNode(uint32 index) const                  { return mEnabledNodes[index]; }

        /**
         * Enable all nodes inside the actor instance.
         * This means that all nodes will be processed and will have their motions sampled (unless disabled by LOD), local and global space matrices calculated, etc.
         * After actor instance creation time it is possible that some nodes are disabled on default. This is controlled by node groups (represented by the NodeGroup class).
         * Each Actor object has a set of node groups. Each group contains a set of nodes which are either enabled or disabled on default.
         * You can use the Actor::GetNodeGroup(...) related methods to get access to the groups and enable or disable the predefined groups of nodes manually.
         */
        void EnableAllNodes();

        /**
         * Disable all nodes inside the actor instance.
         * This means that no nodes will be updated at all (no motion sampling, no transformation calculations and no blending etc).
         * After actor instance creation time it is possible that some nodes are disabled on default. This is controlled by node groups (represented by the NodeGroup class).
         * Each Actor object has a set of node groups. Each group contains a set of nodes which are either enabled or disabled on default.
         * You can use the Actor::GetNodeGroup(...) related methods to get access to the groups and enable or disable the predefined groups of nodes manually.
         */
        void DisableAllNodes();

        uint32 GetThreadIndex() const;
        void SetThreadIndex(uint32 index);

        void DrawSkeleton(Pose& pose, uint32 color);
        void ApplyMotionExtractionDelta(const Transform& trajectoryDelta);
        void ApplyMotionExtractionDelta();
        void MotionExtractionCompensate(EMotionExtractionFlags motionExtractionFlags=(EMotionExtractionFlags)0);
        void MotionExtractionCompensate(Transform& inOutMotionExtractionNodeTransform, EMotionExtractionFlags motionExtractionFlags=(EMotionExtractionFlags)0);

        void SetMotionExtractionEnabled(bool enabled);
        bool GetMotionExtractionEnabled() const;

        void SetTrajectoryDeltaTransform(const Transform& transform);
        const Transform& GetTrajectoryDeltaTransform() const;

        void SetEyeBlinker(EyeBlinker* blinker);
        EyeBlinker* GetEyeBlinker() const;

        MCore::AttributeSet* GetAttributeSet() const;

        AnimGraphPose* RequestPose(uint32 threadIndex);
        void FreePose(uint32 threadIndex, AnimGraphPose* pose);

        /**
         * Get the global space pose blend buffer.
         * This is used to blend between global space controllers.
         * Global space controllers will output into this global pose object, on which blending is performed to smoothly
         * fade in and fade out global space controllers when they get activated or deactivated.
         * If you are trying to get the global space transformations for a node, use the TransformData class instead.
         * @result A pointer to the global space pose blend buffer.
         */
        GlobalPose* GetGlobalPose() const;

        void SetMotionSamplingTimer(float timeInSeconds);
        void SetMotionSamplingRate(float updateRateInSeconds);
        float GetMotionSamplingTimer() const;
        float GetMotionSamplingRate() const;

        MCORE_INLINE uint32 GetNumNodes() const         { return mActor->GetSkeleton()->GetNumNodes(); }

        void UpdateVisualizeScale();                    // not automatically called on creation for performance reasons (this method relatively is slow as it updates all meshes)
        float GetVisualizeScale() const;
        void SetVisualizeScale(float factor);
 

    private:
        TransformData*          mTransformData;         /**< The transformation data for this instance. */
        MCore::AABB             mAABB;                  /**< The axis aligned bounding box. */
        MCore::AABB             mStaticAABB;            /**< A static pre-calculated bounding box, which we can move along with the position of the actor instance, and use for visibility checks. */

        Transform               mLocalTransform;
        Transform               mGlobalTransform;
        Transform               mParentGlobalTransform;
        Transform               mTrajectoryDelta;

        MCore::AttributeSet*    mAttributeSet;          /**< The attribute set, to store custom data. */

        MCore::Array<Attachment*>               mAttachments;       /**< The attachments linked to this actor instance. */
        MCore::Array<Actor::Dependency>         mDependencies;      /**< The actor dependencies, which specify which Actor objects this instance is dependent on. */
        MCore::Array<LocalSpaceController*>     mLocalControllers;  /**< The collection of controllers that have been added to this actor instance. */
        MCore::Array<GlobalSpaceController*>    mGlobalControllers; /**< The collection of global space controllers. */
        MorphSetupInstance*                     mMorphSetup;        /**< The  morph setup instance. */
        MCore::Array<uint16>                    mEnabledNodes;      /**< The list of nodes that are enabled. */

        Actor*                  mActor;                 /**< A pointer to the parent actor where this is an instance from. */
        ActorInstance*          mAttachedTo;            /**< Specifies the actor where this actor is attached to, or nullptr when it is no attachment. */
        Attachment*             mSelfAttachment;        /**< The attachment it is itself inside the mAttachedTo actor instance, or nullptr when this isn't an attachment. */
        MotionSystem*           mMotionSystem;          /**< The motion system, that handles all motion playback and blending etc. */
        EyeBlinker*             mEyeBlinker;            /**< A procedural eyeblinker, can be nullptr. */
        AnimGraphInstance*      mAnimGraphInstance;     /**< A pointer to the anim graph instance, which can be nullptr when there is no anim graph instance. */
        GlobalPose*             mGlobalPose;            /**< The global pose. */
        MCore::Mutex            mLock;                  /**< The multithread lock. */
        void*                   mCustomData;            /**< A pointer to custom data for this actor. This could be a pointer to your engine or game object for example. */
        AZ::Uuid                mCustomDataType;        /**< Type Id of custom data assigned to this actor instance. */
        float                   mBoundsUpdateFrequency; /**< The bounds update frequency. Which is a time value in seconds. */
        float                   mBoundsUpdatePassedTime;/**< The time passed since the last bounds update. */
        float                   mMotionSamplingRate;    /**< The motion sampling rate in seconds, where 0.1 would mean to update 10 times per second. A value of 0 or lower means to update every frame. */
        float                   mMotionSamplingTimer;   /**< The time passed since the last time we sampled motions/anim graphs. */
        float                   mVisualizeScale;        /**< Some visualization scale factor when rendering for example normals, to be at a nice size, relative to the character. */
        uint32                  mLODLevel;              /**< The current LOD level, where 0 is the highest detail. */
        uint32                  mBoundsUpdateItemFreq;  /**< The bounds update item counter step size. A value of 1 means every vertex/node, a value of 2 means every second vertex/node, etc. */
        uint32                  mID;                    /**< The unique identification number for the actor instance. */
        uint32                  mThreadIndex;           /**< The thread index. This specifies the thread number this actor instance is being processed in. */
        EBoundsType             mBoundsUpdateType;      /**< The bounds update type (node based, mesh based or colliison mesh based). */
        uint8                   mNumAttachmentRefs;     /**< Specifies how many actor instances use this actor instance as attachment. */
        uint8                   mBoolFlags;             /**< Boolean flags. */

        /**
         * Boolean masks, as replacement for having several bools as members.
         */
        enum
        {
            BOOL_BOUNDSUPDATEENABLED    = 1 << 0,   /**< Should we automatically update bounds for this node? */
            BOOL_ISVISIBLE              = 1 << 1,   /**< Is this node visible? */
            BOOL_RENDER                 = 1 << 2,   /**< Should this actor instance trigger the OnRender callback method? */
            BOOL_NORMALIZEDMOTIONLOD    = 1 << 3,   /**< Use normalized motion LOD maximum error values? */
            BOOL_USEDFORVISUALIZATION   = 1 << 4,   /**< Indicates if the actor is used for visualization specific things and is not used as a normal in-game actor. */
            BOOL_ENABLED                = 1 << 5,   /**< Exclude the actor instance from the scheduled updates? If so, it is like the actor instance won't exist. */
            BOOL_MOTIONEXTRACTION       = 1 << 6,   /**< Enabled when motion extraction should be active on this actor instance. This still requires the Actor to have a valid motion extraction node setup, and individual motion instances having motion extraction enabled as well. */

#if defined(EMFX_DEVELOPMENT_BUILD)
            BOOL_OWNEDBYRUNTIME         = 1 << 7    /**< Set if the actor instance is used/owned by the engine runtime. */
#endif // EMFX_DEVELOPMENT_BUILD
        };

        /**
         * The constructor.
         * @param actor The actor where this actor instance should be created from.
         * @param threadIndex The thread index to create the instance on.
         */
        ActorInstance(Actor* actor, uint32 threadIndex = 0);

        /**
         * The destructor.
         * This automatically unregisters it from the actor manager as well.
         */
        ~ActorInstance();

        /**
         * Increase the attachment reference count.
         * The number of attachment refs represents how many times this actor instance is an attachment.
         * This is only allowed to be one. This is used for debugging mostly, to prevent you from adding the same attachment
         * to multiple actor instances.
         * @param numToIncreaseWith The number to increase the counter with.
         */
        void IncreaseNumAttachmentRefs(uint8 numToIncreaseWith = 1);

        /**
         * Increase the attachment reference count.
         * The number of attachment refs represents how many times this actor instance is an attachment.
         * This is only allowed to be one. This is used for debugging mostly, to prevent you from adding the same attachment
         * to multiple actor instances.
         * @param numToDecreaseWith The number to decrease the counter with.
         */
        void DecreaseNumAttachmentRefs(uint8 numToDecreaseWith = 1);

        /**
         * Get the number of attachment references.
         * This number represents how many times this actor instance itself is an attachment.
         * This value is mainly used for debugging, as you are not allowed to attach the same actor instance to muliple other actor instances.
         * @result The number of times that this attachment is an attachment in another actor instance.
         */
        uint8 GetNumAttachmentRefs() const;

        /**
         * Set the actor instance where we are attached to.
         * Please note that if you want to set an attachment yourself, you have to use the ActorInstance::AddAttachment().
         * That method will handle setting all such values like you set with this method.
         * @param actorInstance The actor instance where we are attached to.
         */
        void SetAttachedTo(ActorInstance* actorInstance);

        /**
         * Set the attachment where this actor instance is part of.
         * So if this actor instance is a gun, and the gun is attached to some cowboy actor instance, then the self attachment object
         * that you specify as parameter here, is the attachment object for the gun that was added to the cowboy actor instance.
         * @param selfAttachment The attachment where this actor instance takes part of.
         */
        void SetSelfAttachment(Attachment* selfAttachment);

        /**
         * Enable boolean flags.
         * @param flag The flags to enable.
         */
        void EnableFlag(uint8 flag);

        /**
         * Disable boolean flags.
         * @param flag The flags to disable.
         */
        void DisableFlag(uint8 flag);

        /**
         * Enable or disable specific flags.
         * @param flag The flags to modify.
         * @param enabled Set to true to enable the flags, or false to disable them.
         */
        void SetFlag(uint8 flag, bool enabled);

        /**
         * Set the skeletal detail level node flags and enable or disable the nodes accordingly.
         * In total there are 32 possible skeletal LOD levels, where 0 is the highest detail, and 31 the lowest detail.
         * Each detail level can disable specific nodes in the actor. These disabled nodes will not be processed at all by EMotion FX.
         * It is important to know that disabled nodes never should be used inside skinning information or on other places where their transformations
         * are needed.
         * @param level The skeletal detail LOD level. Values higher than 31 will be automatically clamped to 31.
         */
        void SetSkeletalLODLevelNodeFlags(uint32 level);
    };
}   // namespace EMotionFX

