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

// include the required headers
#include <MCore/Source/WaveletHelper.h>
#include <MCore/Source/HaarWavelet.h>
#include <MCore/Source/Endian.h>
#include "WaveletSkeletalMotion.h"
#include "SkeletalSubMotion.h"
#include "MorphSubMotion.h"
#include "SkeletalMotion.h"
#include "MotionInstance.h"
#include "ActorInstance.h"
#include "TransformData.h"
#include "MorphSetupInstance.h"
#include "MotionEventTable.h"
#include "Node.h"
#include "EventManager.h"


namespace EMotionFX
{
    //-------------------------------------------------------------------------------------------------
    // class WaveletSkeletalMotion::Chunk
    //-------------------------------------------------------------------------------------------------

    // constructor
    WaveletSkeletalMotion::Chunk::Chunk()
        : BaseObject()
    {
        mCompressedRotData          = nullptr;
        mCompressedPosData          = nullptr;
        mCompressedMorphData        = nullptr;
        mCompressedRotNumBytes      = 0;
        mCompressedPosNumBytes      = 0;
        mCompressedMorphNumBytes    = 0;
        mRotQuantScale              = 1.0f;
        mPosQuantScale              = 1.0f;
        mMorphQuantScale            = 1.0f;
        mCompressedPosNumBits       = 0;
        mCompressedRotNumBits       = 0;
        mCompressedMorphNumBits     = 0;
        mStartTime                  = 0.0f;

        EMFX_SCALECODE
        (
            mCompressedScaleNumBytes    = 0;
            mCompressedScaleNumBits     = 0;
            mCompressedScaleData        = nullptr;
            mScaleQuantScale            = 1.0f;
        )
    }


    // destructor
    WaveletSkeletalMotion::Chunk::~Chunk()
    {
        MCore::Free(mCompressedRotData);
        MCore::Free(mCompressedPosData);
        MCore::Free(mCompressedMorphData);
        EMFX_SCALECODE(MCore::Free(mCompressedScaleData);
            )
    }


    // create
    WaveletSkeletalMotion::Chunk* WaveletSkeletalMotion::Chunk::Create()
    {
        return new Chunk();
    }


    //-------------------------------------------------------------------------------------------------
    // class WaveletSkeletalMotion
    //-------------------------------------------------------------------------------------------------

    // constructor
    WaveletSkeletalMotion::WaveletSkeletalMotion(const char* name)
        : SkeletalMotion(name)
    {
        mChunks.SetMemoryCategory(EMFX_MEMCATEGORY_WAVELETSKELETONMOTION);
        mWavelet                    = WAVELET_HAAR;
        mDecompressedPosNumBytes    = 0;
        mDecompressedRotNumBytes    = 0;
        mDecompressedMorphNumBytes  = 0;
        mSampleSpacing              = 0.0f;
        mSamplesPerChunk            = 0;
        mNumRotTracks               = 0;
        mNumPosTracks               = 0;
        mNumMorphTracks             = 0;
        mSecondsPerChunk            = 0.0f;
        mChunkOverhead              = 0;
        mCompressedSize             = 0;
        mOptimizedSize              = 0;
        mUncompressedSize           = 0;
        mPosQuantizeFactor          = 1;
        mRotQuantizeFactor          = 1;
        mMorphQuantizeFactor        = 1;
        mScale                      = 1.0f;

        EMFX_SCALECODE
        (
            mScaleQuantizeFactor        = 1;
            //mScaleRotOffset               = 0;
            mNumScaleTracks             = 0;
            //mNumScaleRotTracks            = 0;
            mDecompressedScaleNumBytes  = 0;
        )
    }


    // destructor
    WaveletSkeletalMotion::~WaveletSkeletalMotion()
    {
        ReleaseData();
        GetWaveletCache().RemoveChunksForMotion(this);
    }


    // create
    WaveletSkeletalMotion* WaveletSkeletalMotion::Create(const char* name)
    {
        return new WaveletSkeletalMotion(name);
    }


    // returns the type ID of the motion
    uint32 WaveletSkeletalMotion::GetType() const
    {
        return WaveletSkeletalMotion::TYPE_ID;
    }


    // returns the type as string
    const char* WaveletSkeletalMotion::GetTypeString() const
    {
        return "WaveletSkeletalMotion";
    }


    // init
    void WaveletSkeletalMotion::Init(SkeletalMotion* skeletalMotion, WaveletSkeletalMotion::Settings* settings)
    {
        // release any existing chunks
        ReleaseData();

        // get the settings, use defaults when none provided
        Settings defaultSettings;
        if (settings == nullptr)
        {
            settings = &defaultSettings;
        }

        // make sure the values are in range
        settings->mPositionQuality  = MCore::Clamp<float>(settings->mPositionQuality, 1.0f, 100.0f);
        settings->mRotationQuality  = MCore::Clamp<float>(settings->mRotationQuality, 1.0f, 100.0f);
        settings->mMorphQuality     = MCore::Clamp<float>(settings->mMorphQuality, 1.0f, 100.0f);
        EMFX_SCALECODE(settings->mScaleQuality     = MCore::Clamp<float>(settings->mScaleQuality, 1.0f, 100.0f));

        mWavelet            = settings->mWavelet;
        mCompressedSize     = 0;
        mUncompressedSize   = 0;
        mOptimizedSize      = 0;
        mUnitType           = skeletalMotion->GetUnitType();

        // calculate the real values we will use
        mChunkOverhead      = 0;
        uint32 numSamples   = settings->mSamplesPerChunk;
        numSamples          = MCore::Math::NextPowerOfTwo(numSamples);

        const float maxChunkDuration = numSamples / (float)settings->mSamplesPerSecond;

        // pre-alloc space for the chunks
        const float maxTime = skeletalMotion->GetMaxTime();
        const uint32 numChunks = (uint32)(maxTime / (float)maxChunkDuration) + 1;
        mChunks.Reserve(numChunks);

        mSecondsPerChunk    = maxChunkDuration;
        mSamplesPerChunk    = numSamples;
        mSampleSpacing      = mSecondsPerChunk / (float)(numSamples - 1);
        mMaxTime            = skeletalMotion->GetMaxTime();

        //MCore::LogInfo("maxChunkSeconds = %f secs in %d samples", maxChunkSeconds, numSamples);
        //------------------------------------------------------------------------------------------------------------
        // transform submotions
        //------------------------------------------------------------------------------------------------------------
        // copy over the submotions
        uint16 posIndex     = 0;
        uint16 rotIndex     = 0;
        EMFX_SCALECODE
        (
            //uint16 scaleRotIndex= 0;
            uint16 scaleIndex   = 0;
        )

        const uint32 numSubMotions = skeletalMotion->GetNumSubMotions();
        mSubMotions.Resize(numSubMotions);
        mSubMotionMap.Resize(numSubMotions);
        for (uint32 i = 0; i < numSubMotions; ++i)
        {
            SkeletalSubMotion* skelSubMotion = skeletalMotion->GetSubMotion(i);
            mSubMotions[i] = SkeletalSubMotion::Create(skelSubMotion->GetName());

            // copy over the bind pose
            mSubMotions[i]->SetBindPosePos(skelSubMotion->GetBindPosePos());
            mSubMotions[i]->SetBindPoseRot(skelSubMotion->GetBindPoseRot());
            EMFX_SCALECODE
            (
                mSubMotions[i]->SetBindPoseScale(skelSubMotion->GetBindPoseScale());
            )

            // copy over the pose
            mSubMotions[i]->SetPosePos(skelSubMotion->GetPosePos());
            mSubMotions[i]->SetPoseRot(skelSubMotion->GetPoseRot());
            EMFX_SCALECODE
            (
                mSubMotions[i]->SetPoseScale(skelSubMotion->GetPoseScale());
            )

            // copy over the motion LOD error
            //mSubMotions[i]->SetMotionLODMaxError( skelSubMotion->GetMotionLODMaxError() );

            // update the map
            mSubMotionMap[i].mPosIndex      = (skelSubMotion->GetPosTrack()     ) ? posIndex++ : MCORE_INVALIDINDEX16;
            mSubMotionMap[i].mRotIndex      = (skelSubMotion->GetRotTrack()     ) ? rotIndex++ : MCORE_INVALIDINDEX16;

            EMFX_SCALECODE
            (
                //mSubMotionMap[i].mScaleRotIndex = /*(skelSubMotion->GetScaleRotTrack()) ? scaleRotIndex++ :*/ MCORE_INVALIDINDEX16;
                mSubMotionMap[i].mScaleIndex    = (skelSubMotion->GetScaleTrack()   ) ? scaleIndex++ : MCORE_INVALIDINDEX16;
            )
        }

        // pre-alloc space used for the compression
        BufferInfo buffers;
        buffers.mUncompressedRotations  = (MCore::Quaternion*)MCore::Allocate(numSamples * sizeof(MCore::Quaternion) * numSubMotions);
        buffers.mUncompressedVectors    = (MCore::Vector3*)MCore::Allocate(numSamples * sizeof(MCore::Vector3) * numSubMotions);
        buffers.mCoeffBuffer            = (float*)MCore::Allocate(sizeof(float) * numSamples * numSubMotions * 4 * 2);
        buffers.mQuantBuffer            = (int16*)MCore::Allocate(sizeof(int16) * numSamples * numSubMotions * 4 * 2);

        mPosQuantizeFactor      = 1 + (100 - settings->mPositionQuality) * 0.4f;
        mRotQuantizeFactor      = 1 + (100 - settings->mRotationQuality) * 0.4f;
        EMFX_SCALECODE(mScaleQuantizeFactor    = 1 + (100 - settings->mScaleQuality) * 0.4f;
            )
        //------------------------------------------------------------------------------------------------------------

        //------------------------------------------------------------------------------------------------------------
        // morph submotions
        //------------------------------------------------------------------------------------------------------------
        // copy over the submotions
        uint16 morphIndex = 0;

        const uint32 numMorphSubMotions = skeletalMotion->GetNumMorphSubMotions();
        mMorphSubMotions.Resize(numMorphSubMotions);
        mMorphSubMotionMap.Resize(numMorphSubMotions);
        for (uint32 i = 0; i < numMorphSubMotions; ++i)
        {
            MorphSubMotion* morphSubMotion = skeletalMotion->GetMorphSubMotion(i);
            mMorphSubMotions[i] = MorphSubMotion::Create(morphSubMotion->GetID());

            // copy over values
            mMorphSubMotions[i]->SetPoseWeight(morphSubMotion->GetPoseWeight());

            // update the map
            mMorphSubMotionMap[i] = (morphSubMotion->GetKeyTrack()) ? morphIndex++ : MCORE_INVALIDINDEX16;
        }

        // update buffers
        buffers.mUncompressedMorphs     = (float*)MCore::Allocate(numSamples * sizeof(float) * numMorphSubMotions);
        mMorphQuantizeFactor            = 1 + (100 - settings->mMorphQuality) * 0.4f;
        //------------------------------------------------------------------------------------------------------------

        // init all chunks again, but without gather mode
        float curStartTime = 0.0f;
        while (curStartTime < maxTime)
        {
            // create the new chunk
            Chunk* newChunk = Chunk::Create();
            mChunks.Add(newChunk);

        #ifndef EMFX_SCALE_DISABLED
            InitChunk(newChunk, skeletalMotion, curStartTime, maxChunkDuration, buffers);
        #else
            InitChunk(newChunk, skeletalMotion, curStartTime, maxChunkDuration, buffers);
        #endif
            curStartTime += maxChunkDuration;
        }

        // free temp buffers
        MCore::Free(buffers.mUncompressedRotations);
        MCore::Free(buffers.mUncompressedVectors);
        MCore::Free(buffers.mUncompressedMorphs);
        MCore::Free(buffers.mCoeffBuffer);
        MCore::Free(buffers.mQuantBuffer);

        // calculate the uncompressed and optimized sizes
        for (uint32 s = 0; s < numSubMotions; ++s)
        {
            SkeletalSubMotion* subMotion = skeletalMotion->GetSubMotion(s);
            if (subMotion->GetRotTrack())
            {
                mUncompressedSize += (uint32)((mSecondsPerChunk * 30.0f) * mChunks.GetLength() * (sizeof(MCore::Quaternion) + sizeof(float)));
                mOptimizedSize += subMotion->GetRotTrack()->GetNumKeys() * (sizeof(MCore::Compressed16BitQuaternion) + sizeof(float));
            }

            if (subMotion->GetPosTrack())
            {
                mUncompressedSize += (uint32)((mSecondsPerChunk * 30.0f) * mChunks.GetLength() * (sizeof(MCore::Vector3) + sizeof(float)));
                mOptimizedSize += subMotion->GetPosTrack()->GetNumKeys() * (sizeof(MCore::Vector3) + sizeof(float));
            }

            EMFX_SCALECODE
            (
                if (subMotion->GetScaleTrack())
                {
                    mUncompressedSize += (uint32)((mSecondsPerChunk * 30.0f) * mChunks.GetLength() * (sizeof(MCore::Vector3) + sizeof(float)));
                    mOptimizedSize += subMotion->GetScaleTrack()->GetNumKeys() * (sizeof(MCore::Vector3) + sizeof(float));
                }
            )
        }

        // take morph targets into account
        for (uint32 s = 0; s < numMorphSubMotions; ++s)
        {
            MorphSubMotion* morphSubMotion = skeletalMotion->GetMorphSubMotion(s);
            if (morphSubMotion->GetKeyTrack())
            {
                mUncompressedSize   += (uint32)((mSecondsPerChunk * 30.0f) * mChunks.GetLength() * (sizeof(float) + sizeof(float)));
                mOptimizedSize      += morphSubMotion->GetKeyTrack()->GetNumKeys() * (sizeof(MCore::Compressed16BitFloat) + sizeof(float));
            }
        }

        mChunkOverhead   = mChunks.GetLength() * sizeof(Chunk);
        mCompressedSize += mChunkOverhead;
        mCompressedSize += mSubMotionMap.GetLength() * sizeof(Mapping);
        mCompressedSize += mMorphSubMotionMap.GetLength() * sizeof(uint16);

        //MCore::LogDetailedInfo("Compressed = %d  - Uncompressed = %d  - Optimized - %d  - Chunk overhead = %d bytes  - numChunks=%d", mCompressedSize, mUncompressedSize, mOptimizedSize, mChunks.GetLength() * sizeof(Chunk), mChunks.GetLength());
        //  MCore::LogDetailedInfo("Compression ratio = %.2f (%.2fk)  -  Compared to optimized ratio = %.2f (%.2fk)  -  30 fps uncompressed = %.2fk", mUncompressedSize / (float)mCompressedSize, mCompressedSize/1000.0f, mOptimizedSize / (float)mCompressedSize, mOptimizedSize/1000.0f, mUncompressedSize / 1000.0f);

        // copy over the motion events
        skeletalMotion->GetEventTable()->CopyTo(mEventTable, this);
    }


    // initialize the chunk
    void WaveletSkeletalMotion::InitChunk(WaveletSkeletalMotion::Chunk* chunk, SkeletalMotion* skelMotion, float startTime, float duration, BufferInfo& buffers)
    {
        MCORE_UNUSED(duration);

        const uint32 numSubMotions = skelMotion->GetNumSubMotions();
        const uint32 numMorphSubMotions = skelMotion->GetNumMorphSubMotions();
        const uint32 numSamples = mSamplesPerChunk;

        MCore::Wavelet* wavelet = GetWaveletCache().GetWavelet(0, (uint32)mWavelet);

        mNumMorphTracks     = 0;
        mNumRotTracks       = 0;
        mNumPosTracks       = 0;
        EMFX_SCALECODE
        (
            mNumScaleRotTracks  = 0;
            mNumScaleTracks     = 0;
        )

        chunk->mStartTime   = startTime;

        // find out the maximum motion LOD error
        uint32 s;
        /*  float lodMaxError = 0.0f;
            for (s=0; s<numSubMotions; ++s)
                if (mSubMotions[s]->GetMotionLODMaxError() > lodMaxError)
                    lodMaxError = mSubMotions[s]->GetMotionLODMaxError();*/

        //---------------------------------
        // process rotations
        //---------------------------------
        uint32 i;
        for (s = 0; s < numSubMotions; ++s)
        {
            SkeletalSubMotion* subMotion = skelMotion->GetSubMotion(s);

            // process the rotation track
            EMotionFX::KeyTrackLinear<MCore::Quaternion, MCore::Compressed16BitQuaternion>* rotTrack = subMotion->GetRotTrack();
            if (rotTrack)
            {
                // build the signal by sampling the original uncompressed motion
                float curTime = startTime;
                for (i = 0; i < numSamples; ++i)
                {
                    buffers.mUncompressedRotations[mNumRotTracks * numSamples + i] = rotTrack->GetValueAtTime(curTime).Normalized();
                    curTime += mSampleSpacing;
                }

                for (i = 0; i < 4; ++i)
                {
                    float* buffer = &buffers.mCoeffBuffer[(mNumRotTracks * numSamples * 4) + i * numSamples];
                    for (uint32 a = 0; a < numSamples; ++a)
                    {
                        buffer[a] = buffers.mUncompressedRotations[mNumRotTracks * numSamples + a][i];
                    }

                    // calculate wavelet coefficients
                    wavelet->Transform(buffer, numSamples);
                }

                mNumRotTracks++;
            }
        } // for all submotions


        //---------------------------------
        // process scale rotations
        //---------------------------------
        /*
        #ifndef EMFX_SCALE_DISABLED
                mScaleRotOffset = numSamples * mNumRotTracks * 4;
                for (s=0; s<numSubMotions; ++s)
                {
                    SkeletalSubMotion* subMotion = skelMotion->GetSubMotion(s);

                    // process scale rotations
                    EMotionFX::KeyTrackLinear<MCore::Quaternion, MCore::Compressed16BitQuaternion>* scaleRotTrack = subMotion->GetScaleRotTrack();
                    if (scaleRotTrack)
                    {
                        // build the signal by sampling the original uncompressed motion
                        float curTime = startTime;
                        for (i=0; i<numSamples; ++i)
                        {
                            buffers.mUncompressedRotations[mNumScaleRotTracks*numSamples+i] = scaleRotTrack->GetValueAtTime( curTime );
                            curTime += mSampleSpacing;
                        }

                        for (i=0; i<4; ++i)
                        {
                            float* buffer = &buffers.mCoeffBuffer[ mScaleRotOffset + (mNumScaleRotTracks*numSamples*4)+i*numSamples];
                            for (uint32 a=0; a<numSamples; ++a)
                                buffer[a] = buffers.mUncompressedRotations[mNumScaleRotTracks*numSamples+a][i];

                            // calculate wavelet coefficients
                            wavelet->Transform( buffer, numSamples );
                        }

                        mNumScaleRotTracks++;
                    }
                } // for all submotions

        #endif
        */
        //---------------------------------
        // quantize and compress rotation data
        //---------------------------------
        // now that we have a big buffer with all wavelet coefficients, quantize it fully
#ifndef EMFX_SCALE_DISABLED
        uint32 quantBufferSize = (numSamples * mNumRotTracks * 4) + (numSamples * mNumScaleRotTracks * 4);
#else
        uint32 quantBufferSize = (numSamples * mNumRotTracks * 4);
#endif
        if (quantBufferSize > 0)
        {
            // quantize the buffer
            MCore::WaveletHelper::Quantize(buffers.mCoeffBuffer, quantBufferSize, buffers.mQuantBuffer, chunk->mRotQuantScale, mRotQuantizeFactor);

            // compress the quantized data
            //if (mCompressor == COMPRESSOR_HUFFMAN)
            GetWaveletCache().GetCompressor().Compress((uint8*)buffers.mQuantBuffer, sizeof(int16) * quantBufferSize, &chunk->mCompressedRotData, &chunk->mCompressedRotNumBytes);
            //else
            //MCore::RiceCompressor::Compress( (uint8*)buffers.mQuantBuffer, quantBufferSize, &chunk->mCompressedRotData, &chunk->mCompressedRotNumBits, &chunk->mCompressedRotNumBytes, mRotQuantizeFactor, 1.0f );

            mCompressedSize += chunk->mCompressedRotNumBytes;
            mDecompressedRotNumBytes = quantBufferSize * sizeof(int16);
        }


        //---------------------------------
        // process positions
        //---------------------------------
        for (s = 0; s < numSubMotions; ++s)
        {
            SkeletalSubMotion* subMotion = skelMotion->GetSubMotion(s);

            // process scale rotations
            EMotionFX::KeyTrackLinear<MCore::Vector3, MCore::Vector3>* posTrack = subMotion->GetPosTrack();
            if (posTrack)
            {
                // build the signal by sampling the original uncompressed motion
                float curTime = startTime;
                for (i = 0; i < numSamples; ++i)
                {
                    buffers.mUncompressedVectors[mNumPosTracks * numSamples + i] = posTrack->GetValueAtTime(curTime) - subMotion->GetPosePos();
                    curTime += mSampleSpacing;
                }

                for (i = 0; i < 3; ++i)
                {
                    float* buffer = &buffers.mCoeffBuffer[ (mNumPosTracks * numSamples * 3) + i * numSamples];
                    for (uint32 a = 0; a < numSamples; ++a)
                    {
                        buffer[a] = buffers.mUncompressedVectors[mNumPosTracks * numSamples + a][i];
                    }

                    // calculate wavelet coefficients
                    wavelet->Transform(buffer, numSamples);
                }

                mNumPosTracks++;
            }
        } // for all submotions


        //---------------------------------
        // quantize and compress position data
        //---------------------------------
        if (mNumPosTracks > 0)
        {
            // now that we have a big buffer with all wavelet coefficients, quantize it fully
            quantBufferSize = numSamples * mNumPosTracks * 3;

            // quantize the buffer
            MCore::WaveletHelper::Quantize(buffers.mCoeffBuffer, quantBufferSize, buffers.mQuantBuffer, chunk->mPosQuantScale, mPosQuantizeFactor);

            // compress the quantized data
            //if (mCompressor == COMPRESSOR_HUFFMAN)
            GetWaveletCache().GetCompressor().Compress((uint8*)buffers.mQuantBuffer, sizeof(int16) * quantBufferSize, &chunk->mCompressedPosData, &chunk->mCompressedPosNumBytes);
            //else
            //MCore::RiceCompressor::Compress( (uint8*)buffers.mQuantBuffer, quantBufferSize, &chunk->mCompressedPosData, &chunk->mCompressedPosNumBits, &chunk->mCompressedPosNumBytes, mPosQuantizeFactor, 1.0f);

            mCompressedSize += chunk->mCompressedPosNumBytes;
            mDecompressedPosNumBytes = quantBufferSize * sizeof(int16);
        }


        //---------------------------------
        // process scales
        //---------------------------------
#ifndef EMFX_SCALE_DISABLED
        for (s = 0; s < numSubMotions; ++s)
        {
            SkeletalSubMotion* subMotion = skelMotion->GetSubMotion(s);

            // process scale rotations
            EMotionFX::KeyTrackLinear<MCore::Vector3, MCore::Vector3>* scaleTrack = subMotion->GetScaleTrack();
            if (scaleTrack)
            {
                // build the signal by sampling the original uncompressed motion
                float curTime = startTime;
                for (i = 0; i < numSamples; ++i)
                {
                    buffers.mUncompressedVectors[mNumScaleTracks * numSamples + i] = scaleTrack->GetValueAtTime(curTime) - subMotion->GetPoseScale();
                    curTime += mSampleSpacing;
                }

                for (i = 0; i < 3; ++i)
                {
                    float* buffer = &buffers.mCoeffBuffer[(mNumScaleTracks * numSamples * 3) + i * numSamples];
                    for (uint32 a = 0; a < numSamples; ++a)
                    {
                        buffer[a] = buffers.mUncompressedVectors[mNumScaleTracks * numSamples + a][i];
                    }

                    // calculate wavelet coefficients
                    wavelet->Transform(buffer, numSamples);
                }

                mNumScaleTracks++;
            }
        } // for all submotions

        //---------------------------------
        // quantize and compress scale data
        //---------------------------------
        if (mNumScaleTracks > 0)
        {
            // now that we have a big buffer with all wavelet coefficients, quantize it fully
            quantBufferSize = numSamples * mNumScaleTracks * 3;

            // quantize the buffer
            MCore::WaveletHelper::Quantize(buffers.mCoeffBuffer, quantBufferSize, buffers.mQuantBuffer, chunk->mScaleQuantScale, mScaleQuantizeFactor);

            // compress the quantized data
            //if (mCompressor == COMPRESSOR_HUFFMAN)
            GetWaveletCache().GetCompressor().Compress((uint8*)buffers.mQuantBuffer, sizeof(int16) * quantBufferSize, &chunk->mCompressedScaleData, &chunk->mCompressedScaleNumBytes);
            //else
            //MCore::RiceCompressor::Compress( (uint8*)buffers.mQuantBuffer, quantBufferSize, &chunk->mCompressedScaleData, &chunk->mCompressedScaleNumBits, &chunk->mCompressedScaleNumBytes, mScaleQuantizeFactor, 1.0f );

            mCompressedSize += chunk->mCompressedScaleNumBytes;
            mDecompressedScaleNumBytes = quantBufferSize * sizeof(int16);
        }
#endif


        //---------------------------------
        // process morphs
        //---------------------------------
        for (s = 0; s < numMorphSubMotions; ++s)
        {
            MorphSubMotion* subMotion = skelMotion->GetMorphSubMotion(s);

            // process scale rotations
            auto keyTrack = subMotion->GetKeyTrack();
            if (keyTrack)
            {
                // build the signal by sampling the original uncompressed motion
                float curTime = startTime;
                for (i = 0; i < numSamples; ++i)
                {
                    buffers.mUncompressedMorphs[mNumMorphTracks * numSamples + i] = keyTrack->GetValueAtTime(curTime) - subMotion->GetPoseWeight();
                    curTime += mSampleSpacing;
                }

                float* buffer = &buffers.mCoeffBuffer[mNumMorphTracks * numSamples];
                for (uint32 a = 0; a < numSamples; ++a)
                {
                    buffer[a] = buffers.mUncompressedMorphs[mNumMorphTracks * numSamples + a];
                }

                // calculate wavelet coefficients
                wavelet->Transform(buffer, numSamples);
                mNumMorphTracks++;
            }
        } // for all morph submotions


        //---------------------------------
        // quantize and compress morph data
        //---------------------------------
        if (mNumMorphTracks > 0)
        {
            // now that we have a big buffer with all wavelet coefficients, quantize it fully
            quantBufferSize = numSamples * mNumMorphTracks * 3;

            // quantize the buffer
            MCore::WaveletHelper::Quantize(buffers.mCoeffBuffer, quantBufferSize, buffers.mQuantBuffer, chunk->mMorphQuantScale, mMorphQuantizeFactor);

            // compress the quantized data
            GetWaveletCache().GetCompressor().Compress((uint8*)buffers.mQuantBuffer, sizeof(int16) * quantBufferSize, &chunk->mCompressedMorphData, &chunk->mCompressedMorphNumBytes);

            mCompressedSize += chunk->mCompressedMorphNumBytes;
            mDecompressedMorphNumBytes = quantBufferSize * sizeof(int16);
        }
    }


    // release skeletal submotions
    void WaveletSkeletalMotion::ReleaseData()
    {
        mDecompressedPosNumBytes    = 0;
        mDecompressedRotNumBytes    = 0;
        mDecompressedMorphNumBytes  = 0;
        EMFX_SCALECODE(mDecompressedScaleNumBytes  = 0;
            )

        const uint32 numChunks = mChunks.GetLength();
        for (uint32 i = 0; i < numChunks; ++i)
        {
            mChunks[i]->Destroy();
        }

        mChunks.Clear();
    }


    // decompress the chunk
    void WaveletSkeletalMotion::DecompressChunk(Chunk* chunk, WaveletCache::DecompressedChunk* targetChunk, uint32 threadIndex)
    {
        MCore::Wavelet* wavelet = GetWaveletCache().GetWavelet(threadIndex, (uint32)mWavelet);

        uint32 i;
        uint32 t;

        targetChunk->mMotion        = this;
        targetChunk->mStartTime     = chunk->mStartTime;
        targetChunk->mNumSamples    = mSamplesPerChunk;
        targetChunk->mSizeInBytes   = sizeof(WaveletCache::DecompressedChunk);

        const uint32 numSamplesTimesTwo     = mSamplesPerChunk << 1;
        const uint32 numSamplesTimesThree   = mSamplesPerChunk * 3;
        const uint32 numSamplesTimesFour    = mSamplesPerChunk << 2;

        if (mNumRotTracks > 0)
        {
            // get the buffer we can decompress in
            uint8* decompressedBuffer = GetWaveletCache().AssureDecompressBufferSize(threadIndex, mDecompressedRotNumBytes);

            // decompress quantized values
            uint32 decompressedNumBytes;
            //if (mCompressor == COMPRESSOR_HUFFMAN)
            GetWaveletCache().GetCompressor().Decompress(chunk->mCompressedRotData, decompressedBuffer, &decompressedNumBytes);
            //else
            /*      {
            #ifndef EMFX_SCALE_DISABLED
                        decompressedNumBytes = (mNumRotTracks+mNumScaleRotTracks) * numSamplesTimesFour;
            #else
                        decompressedNumBytes = mNumRotTracks * numSamplesTimesFour;
            #endif

                        //MCore::RiceCompressor::Decompress( chunk->mCompressedRotData, decompressedNumBytes, chunk->mCompressedRotNumBits, decompressedBuffer, mRotQuantizeFactor );
                        decompressedNumBytes <<= 1;
                    }
            */

            // dequantize
            const uint32 totalRotSamples = decompressedNumBytes >> 1;
            float* decompressedRotData = GetWaveletCache().AssureDataBufferSize(threadIndex, totalRotSamples);
            MCore::WaveletHelper::Dequantize((int16*)decompressedBuffer, totalRotSamples, decompressedRotData, chunk->mRotQuantScale, mRotQuantizeFactor);
            targetChunk->mSizeInBytes += totalRotSamples * sizeof(float);

            // inverse wavelet transform rotations
            if (mNumRotTracks > 0)
            {
                MCore::Quaternion quat;
                targetChunk->mRotations = (MCore::Compressed16BitQuaternion*)MCore::Allocate(sizeof(MCore::Compressed16BitQuaternion) * targetChunk->mNumSamples * mNumRotTracks);

                for (t = 0; t < mNumRotTracks; ++t)
                {
                    for (i = 0; i < 4; ++i)
                    {
                        wavelet->InverseTransform(&decompressedRotData[ (t * numSamplesTimesFour) + (i * mSamplesPerChunk)], mSamplesPerChunk);
                    }

                    for (i = 0; i < mSamplesPerChunk; ++i)
                    {
                        const uint32 offset = (t * numSamplesTimesFour) + i;
                        quat.x = decompressedRotData[offset];
                        quat.y = decompressedRotData[offset + mSamplesPerChunk];
                        quat.z = decompressedRotData[offset + numSamplesTimesTwo];
                        quat.w = decompressedRotData[offset + numSamplesTimesThree];
                        quat.Normalize();
                        targetChunk->mRotations[(t * mSamplesPerChunk) + i].FromQuaternion(quat);
                    }
                }
            }
            /*
            #ifndef EMFX_SCALE_DISABLED
                    // inverse wavelet transform scale rotations
                    if (mNumScaleRotTracks > 0)
                    {
                        Quaternion quat;
                        targetChunk->mScaleRotations = (MCore::Compressed16BitQuaternion*)MCore::Allocate( sizeof(MCore::Compressed16BitQuaternion) * targetChunk->mNumSamples * mNumScaleRotTracks);
                        for (t=0; t<mNumScaleRotTracks; ++t)
                        {
                            for (i=0; i<4; ++i)
                                wavelet->InverseTransform( &decompressedRotData[mScaleRotOffset + (t*numSamplesTimesFour) + (i*mSamplesPerChunk)], mSamplesPerChunk );

                            for (i=0; i<mSamplesPerChunk; ++i)
                            {
                                const uint32 offset = mScaleRotOffset + (t * numSamplesTimesFour) + i;
                                quat.x = decompressedRotData[offset];
                                quat.y = decompressedRotData[offset+mSamplesPerChunk];
                                quat.z = decompressedRotData[offset+numSamplesTimesTwo];
                                quat.w = decompressedRotData[offset+numSamplesTimesThree];
                                quat.Normalize();
                                MCore::GetCoordinateSystem().ConvertQuaternion( &quat );
                                targetChunk->mScaleRotations[(t*mSamplesPerChunk)+i].FromQuaternion( quat );
                            }
                        }
                    }
            #endif  // EMFX_SCALE_DISABLED
            */
        }


        //--------------------------------------------------------
        // positions
        //--------------------------------------------------------
        if (mNumPosTracks > 0)
        {
            // get the buffer we can decompress in
            uint8* decompressedBuffer = GetWaveletCache().AssureDecompressBufferSize(threadIndex, mDecompressedPosNumBytes);

            // decompress quantized values
            uint32 decompressedPosNumBytes;
            //if (mCompressor == COMPRESSOR_HUFFMAN)
            GetWaveletCache().GetCompressor().Decompress(chunk->mCompressedPosData, decompressedBuffer, &decompressedPosNumBytes);
            //else
            /*      {
                        decompressedPosNumBytes = mNumPosTracks * numSamplesTimesThree;
                        MCore::RiceCompressor::Decompress( chunk->mCompressedPosData, decompressedPosNumBytes, chunk->mCompressedPosNumBits, decompressedBuffer, mPosQuantizeFactor );
                        decompressedPosNumBytes <<= 1;
                    }*/

            // dequantize
            const uint32 totalPosSamples = decompressedPosNumBytes >> 1;
            float* decompressedPosData = GetWaveletCache().AssureDataBufferSize(threadIndex, totalPosSamples);
            MCore::WaveletHelper::Dequantize((int16*)decompressedBuffer, totalPosSamples, decompressedPosData, chunk->mPosQuantScale, mPosQuantizeFactor);
            targetChunk->mSizeInBytes += totalPosSamples * sizeof(float);

            MCore::Vector3 pos;
            targetChunk->mPositions = (MCore::Vector3*)MCore::Allocate(sizeof(MCore::Vector3) * targetChunk->mNumSamples * mNumPosTracks);
            for (t = 0; t < mNumPosTracks; ++t)
            {
                for (i = 0; i < 3; ++i)
                {
                    wavelet->InverseTransform(&decompressedPosData[(t * numSamplesTimesThree) + (i * mSamplesPerChunk)], mSamplesPerChunk);
                }

                for (i = 0; i < mSamplesPerChunk; ++i)
                {
                    const uint32 offset = (t * numSamplesTimesThree) + i;
                    pos.x = decompressedPosData[offset];
                    pos.y = decompressedPosData[offset + mSamplesPerChunk];
                    pos.z = decompressedPosData[offset + numSamplesTimesTwo];
                    targetChunk->mPositions[(t * mSamplesPerChunk) + i] = pos * mScale;
                }
            }
        }

#ifndef EMFX_SCALE_DISABLED
        //--------------------------------------------------------
        // scales
        //--------------------------------------------------------
        if (mNumScaleTracks > 0)
        {
            // get the buffer we can decompress in
            uint8* decompressedBuffer = GetWaveletCache().AssureDecompressBufferSize(threadIndex, mDecompressedScaleNumBytes);

            // decompress quantized values
            uint32 decompressedScaleNumBytes;
            //if (mCompressor == COMPRESSOR_HUFFMAN)
            GetWaveletCache().GetCompressor().Decompress(chunk->mCompressedScaleData, decompressedBuffer, &decompressedScaleNumBytes);
            //else
            /*      {
                        decompressedScaleNumBytes = mNumScaleTracks * mSamplesPerChunk * 3;
                        MCore::RiceCompressor::Decompress( chunk->mCompressedScaleData, decompressedScaleNumBytes, chunk->mCompressedScaleNumBits, decompressedBuffer, mScaleQuantizeFactor );
                        decompressedScaleNumBytes <<= 1;
                    }*/

            // dequantize
            const uint32 totalScaleSamples = decompressedScaleNumBytes >> 1;
            float* decompressedScaleData = GetWaveletCache().AssureDataBufferSize(threadIndex, totalScaleSamples);
            MCore::WaveletHelper::Dequantize((int16*)decompressedBuffer, totalScaleSamples, decompressedScaleData, chunk->mScaleQuantScale, mScaleQuantizeFactor);
            targetChunk->mSizeInBytes += totalScaleSamples * sizeof(float);

            MCore::Vector3 scale;
            targetChunk->mScales = (MCore::Vector3*)MCore::Allocate(sizeof(MCore::Vector3) * targetChunk->mNumSamples * mNumScaleTracks);
            for (t = 0; t < mNumScaleTracks; ++t)
            {
                for (i = 0; i < 3; ++i)
                {
                    wavelet->InverseTransform(&decompressedScaleData[(t * numSamplesTimesThree) + (i * mSamplesPerChunk)], mSamplesPerChunk);
                }

                for (i = 0; i < mSamplesPerChunk; ++i)
                {
                    const uint32 offset = (t * numSamplesTimesThree) + i;
                    scale.x = decompressedScaleData[offset];
                    scale.y = decompressedScaleData[offset + mSamplesPerChunk];
                    scale.z = decompressedScaleData[offset + numSamplesTimesTwo];
                    targetChunk->mScales[(t * mSamplesPerChunk) + i] = scale;
                }
            }
        }
#endif  // EMFX_SCALE_DISABLED

        //--------------------------------------------------------
        // morph weights
        //--------------------------------------------------------
        if (mNumMorphTracks > 0)
        {
            // get the buffer we can decompress in
            uint8* decompressedBuffer = GetWaveletCache().AssureDecompressBufferSize(threadIndex, mDecompressedMorphNumBytes);

            // decompress quantized values
            uint32 decompressedMorphNumBytes;
            GetWaveletCache().GetCompressor().Decompress(chunk->mCompressedMorphData, decompressedBuffer, &decompressedMorphNumBytes);

            // dequantize
            const uint32 totalMorphSamples = decompressedMorphNumBytes >> 1;
            float* decompressedMorphData = GetWaveletCache().AssureDataBufferSize(threadIndex, totalMorphSamples);
            MCore::WaveletHelper::Dequantize((int16*)decompressedBuffer, totalMorphSamples, decompressedMorphData, chunk->mMorphQuantScale, mMorphQuantizeFactor);
            targetChunk->mSizeInBytes += totalMorphSamples * sizeof(float);

            float morphWeight;
            targetChunk->mMorphWeights = (float*)MCore::Allocate(sizeof(float) * targetChunk->mNumSamples * mNumMorphTracks);
            for (t = 0; t < mNumMorphTracks; ++t)
            {
                wavelet->InverseTransform(&decompressedMorphData[t * mSamplesPerChunk], mSamplesPerChunk);

                for (i = 0; i < mSamplesPerChunk; ++i)
                {
                    const uint32 offset = (t * mSamplesPerChunk) + i;
                    morphWeight = decompressedMorphData[offset];
                    targetChunk->mMorphWeights[(t * mSamplesPerChunk) + i] = morphWeight;
                }
            }
        }
    }


    // update the maximum time, but do nothing really here, as its already calculated upfront
    void WaveletSkeletalMotion::UpdateMaxTime()
    {
    }


    // find the right chunk number at a given time
    WaveletSkeletalMotion::Chunk* WaveletSkeletalMotion::FindChunkAtTime(float timeValue) const
    {
        // for all chunks
        //const uint32 numChunks = mChunks.GetLength();
        uint32 chunkNumber = (uint32)(timeValue / mSecondsPerChunk);
        if (chunkNumber < mChunks.GetLength())
        {
            return mChunks[chunkNumber];
        }

        return nullptr;
    }


    // the update method
    // this calculates the output transformations for all nodes
    void WaveletSkeletalMotion::Update(const Pose* inPose, Pose* outPose, MotionInstance* instance)
    {
        ActorInstance*  actorInstance   = instance->GetActorInstance();
        Actor*          actor           = actorInstance->GetActor();
        TransformData*  transformData   = actorInstance->GetTransformData();
        const Pose*     bindPose        = transformData->GetBindPose();

        //EMFX_SCALECODE
        //(
        //      Transform*  orgTransforms   = transformData->GetBindPoseLocalTransforms();
        //  )

        // make sure the pose has the same number of transforms
        MCORE_ASSERT(outPose->GetNumTransforms() == actor->GetNumNodes());

        // get the time value
        const float timeValue = instance->GetCurrentTime();

        // get the mirror data
        //const Vector3 mirrorPlaneNormal   = instance->GetMirrorPlaneNormal();
        const uint32 threadIndex = actorInstance->GetThreadIndex();

        // get the chunk of data we need and decompress it if needed
        //GetWaveletCache().LockChunks();
        WaveletCache::DecompressedChunk* decompressedChunk = GetWaveletCache().GetChunkAtTime(timeValue, this, threadIndex);

        // calculate the interpolation time value and keys
        float interpolateT;
        uint32 firstSampleIndex;
        uint32 secondSampleIndex;
        decompressedChunk->CalcInterpolationValues(timeValue, &firstSampleIndex, &secondSampleIndex, &interpolateT);

        //
        Transform sampledTransform;

        // process all motion links
        Transform outTransform;
        const uint32 numNodes = actorInstance->GetNumEnabledNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            const uint32    nodeNumber   = actorInstance->GetEnabledNode(i);
            MotionLink*     link         = instance->GetMotionLink(nodeNumber);
            //Node*         node         = actor->GetNode(nodeNumber);
            //Transform*        outTransform = &outPose->GetLocalTransformDirect(nodeNumber);
            //      const Actor::NodeMirrorInfo& mirrorInfo = actor->GetNodeMirrorInfo(nodeNumber);

            // if there is no submotion linked to this node
            if (link->GetIsActive() == false)
            {
                outTransform = inPose->GetLocalTransform(nodeNumber);
                outPose->SetLocalTransform(nodeNumber, outTransform);
                continue;
            }

            // get the current time and the submotion
            const uint32 subMotionIndex = link->GetSubMotionIndex();
            SkeletalSubMotion* subMotion = mSubMotions[ subMotionIndex ];

            decompressedChunk->GetTransformAtTime(subMotionIndex, interpolateT, firstSampleIndex, secondSampleIndex, &outTransform);

            // perform basic retargeting
            if (instance->GetRetargetingEnabled())
            {
                const Transform& bindTransform = bindPose->GetLocalTransform(nodeNumber);
                MCore::Vector3 posOffset(0.0f, 0.0f, 0.0f);
                MCore::Vector3 nodeOrgPos = bindTransform.mPosition;
                /*
                            // if we deal with the root node
                            if (instance->GetRetargetRootIndex() == nodeNumber)
                            {
                                const float bindPoseHeight = subMotion->GetBindPosePos().y;
                                if (Math::Abs(bindPoseHeight) > 0.0001f)
                                {
                                    posOffset.x = 0.0f;
                                    posOffset.y = instance->GetRetargetRootOffset() * MCore::Clamp<float>(outTransform->mPosition.y / bindPoseHeight, 0.0f, 1.0f);
                                    posOffset.z = 0.0f;
                                    //posOffset = Vector3(0.0f, instance->GetRetargetRootOffset() * (outTransform->mPosition.y / nodeOrgPos.y), 0.0f);
                                }
                                else
                                    posOffset = Vector3(0.0f, instance->GetRetargetRootOffset(), 0.0f);
                            }
                            else
                                posOffset = nodeOrgPos - subMotion->GetBindPosePos();
                */

                /*          // if we deal with the root node
                            if (actor->GetRetargetRootIndex() == nodeNumber)
                            {
                                const int32 upIndex = MCore::GetCoordinateSystem().GetUpIndex();
                                if (Math::Abs(nodeOrgPos[upIndex]) > 0.0001f)
                                    posOffset[upIndex] = actor->GetRetargetOffset() * (outTransform->mPosition[upIndex] / nodeOrgPos[upIndex]);
                                else
                                    posOffset[upIndex] = actor->GetRetargetOffset();
                            }
                            else*/
                posOffset = nodeOrgPos - subMotion->GetBindPosePos();

                // calculate the displacement
                //          Vector3 posOffset   = orgTransforms[nodeNumber].mPosition - subMotion->GetBindPosePos();

                EMFX_SCALECODE
                (
                    MCore::Vector3 scaleOffset = bindTransform.mScale - subMotion->GetBindPoseScale();
                    outTransform.mScale += scaleOffset;
                )

                // apply the displacements
                outTransform.mPosition  += posOffset;
            } // if retargeting enabled

            outPose->SetLocalTransform(nodeNumber, outTransform);
        } // for all transforms

        // mirror
        if (instance->GetMirrorMotion() && actor->GetHasMirrorInfo())
        {
            MirrorPose(outPose, instance);
        }

        // output the morph targets
        MorphSetupInstance* morphSetup = actorInstance->GetMorphSetupInstance();
        const uint32 numMorphTargets = morphSetup->GetNumMorphTargets();
        for (uint32 i = 0; i < numMorphTargets; ++i)
        {
            const uint32 morphTargetID = morphSetup->GetMorphTarget(i)->GetID();
            const uint32 subMotionIndex = FindMorphSubMotionByID(morphTargetID);
            if (subMotionIndex != MCORE_INVALIDINDEX32)
            {
                float outWeight;
                decompressedChunk->GetMorphWeightAtTime(subMotionIndex, interpolateT, firstSampleIndex, secondSampleIndex, &outWeight);
                outPose->SetMorphWeight(i, outWeight);
            }
            else
            {
                outPose->SetMorphWeight(i, inPose->GetMorphWeight(i));
            }
        }

        //  GetWaveletCache().UnlockChunks();
        //  GetWaveletCache().Shrink( nullptr );
    }



    // calculate the node transform
    void WaveletSkeletalMotion::CalcNodeTransform(const MotionInstance* instance, Transform* outTransform, Actor* actor, Node* node, float timeValue, bool enableRetargeting)
    {
        // get some required shortcuts
        ActorInstance*  actorInstance   = instance->GetActorInstance();
        TransformData*  transformData   = actorInstance->GetTransformData();
        const Pose*     bindPose        = transformData->GetBindPose();

        //EMFX_SCALECODE
        //(
        //      Transform*  orgTransforms   = transformData->GetBindPoseLocalTransforms();
        //  )

        // get the node index/number
        const uint32 nodeIndex      = node->GetNodeIndex();
        const uint32 threadIndex    = actorInstance->GetThreadIndex();

        // get the chunk of data we need and decompress it if needed
        WaveletCache::DecompressedChunk* decompressedChunk = GetWaveletCache().GetChunkAtTime(timeValue, this, threadIndex);

        // search for the motion link
        const MotionLink* motionLink = instance->GetMotionLink(nodeIndex);
        if (motionLink->GetIsActive() == false)
        {
            *outTransform = transformData->GetCurrentPose()->GetLocalTransform(nodeIndex);
            return;
        }

        // get the current time and the submotion
        SkeletalSubMotion* subMotion = mSubMotions[ motionLink->GetSubMotionIndex() ];

        // get the transformation for the given node
        decompressedChunk->GetTransformAtTime(motionLink->GetSubMotionIndex(), timeValue, outTransform);

        // if we want to use retargeting
        if (enableRetargeting)
        {
            const Transform& bindTransform = bindPose->GetLocalTransform(nodeIndex);

            MCore::Vector3 posOffset(0, 0, 0);
            MCore::Vector3 nodeOrgPos = bindTransform.mPosition;

            /*      // if we deal with the root node
                    if (actor->GetRetargetRootIndex() == nodeIndex)
                    {
                        const int32 upIndex = MCore::GetCoordinateSystem().GetUpIndex();
                        if (Math::Abs(nodeOrgPos[upIndex]) > 0.0001f)
                            posOffset[upIndex] = actor->GetRetargetOffset() * (outTransform->mPosition[upIndex] / nodeOrgPos[upIndex]);
                        else
                            posOffset[upIndex] = actor->GetRetargetOffset();
                    }
                    else*/
            posOffset = nodeOrgPos - subMotion->GetBindPosePos();

            // calculate the displacement
            //Vector3 posOffset   = orgTransforms[nodeIndex].mPosition - subMotion->GetBindPosePos();

            EMFX_SCALECODE
            (
                MCore::Vector3 scaleOffset = bindTransform.mScale - subMotion->GetBindPoseScale();
                outTransform->mScale += scaleOffset;
            )

            // apply the displacements
            outTransform->mPosition += posOffset;
        }

        // mirror
        if (instance->GetMirrorMotion() && actor->GetHasMirrorInfo())
        {
            const Actor::NodeMirrorInfo& mirrorInfo = actor->GetNodeMirrorInfo(nodeIndex);
            Transform mirrored = bindPose->GetLocalTransform(nodeIndex);
            MCore::Vector3 mirrorAxis(0.0f, 0.0f, 0.0f);
            mirrorAxis[mirrorInfo.mAxis] = 1.0f;
            const uint32 motionSource = actor->GetNodeMirrorInfo(nodeIndex).mSourceNode;
            mirrored.ApplyDeltaMirrored(bindPose->GetLocalTransform(motionSource), *outTransform, mirrorAxis, mirrorInfo.mFlags);
            *outTransform = mirrored;
        }
    }


    // swap the chunk data's endianness
    // this needs to uncompress the quantized data, then swap the endian of the uncompressed data, and then recompress and update the buffers and their sizes
    void WaveletSkeletalMotion::SwapChunkDataEndian()
    {
        uint8* decompressedBuffer = nullptr;
        uint32 decompressedNumBytes = 0;

        const uint32 numChunks = mChunks.GetLength();
        for (uint32 c = 0; c < numChunks; ++c)
        {
            Chunk* curChunk = mChunks[c];

            // rotations and scale rotations
    #ifndef EMFX_SCALE_DISABLED
            if (mNumRotTracks > 0 || mNumScaleRotTracks > 0)
        #else
            if (mNumRotTracks > 0)
    #endif
            {
                // decompress rotations
                const uint32 requiredSize = GetWaveletCache().GetCompressor().CalcDecompressedSize(curChunk->mCompressedRotData, curChunk->mCompressedRotNumBytes);
                if (decompressedNumBytes < requiredSize)
                {
                    decompressedBuffer = (uint8*)MCore::Realloc(decompressedBuffer, requiredSize + 4096, EMFX_MEMCATEGORY_MOTIONS_WAVELETSKELETALMOTIONS);
                    decompressedNumBytes = requiredSize + 4096; // 4kb extra allocated already
                }

                uint32 numBytes = 0;
                GetWaveletCache().GetCompressor().Decompress(curChunk->mCompressedRotData, decompressedBuffer, &numBytes);
                MCORE_ASSERT(numBytes == requiredSize);

                // convert endian
                MCore::Endian::ConvertSignedInt16((int16*)decompressedBuffer, numBytes / 2);

                // recompress
                MCore::Free(curChunk->mCompressedRotData);
                GetWaveletCache().GetCompressor().Compress(decompressedBuffer, numBytes, &curChunk->mCompressedRotData, &curChunk->mCompressedRotNumBytes, false);
                curChunk->mCompressedRotNumBits = curChunk->mCompressedRotNumBytes * 8;
            }


            // positions
            if (mNumPosTracks > 0)
            {
                // decompress
                const uint32 requiredSize = GetWaveletCache().GetCompressor().CalcDecompressedSize(curChunk->mCompressedPosData, curChunk->mCompressedPosNumBytes);
                if (decompressedNumBytes < requiredSize)
                {
                    decompressedBuffer = (uint8*)MCore::Realloc(decompressedBuffer, requiredSize + 4096, EMFX_MEMCATEGORY_MOTIONS_WAVELETSKELETALMOTIONS);
                    decompressedNumBytes = requiredSize + 4096; // 4kb extra allocated already
                }

                uint32 numBytes = 0;
                GetWaveletCache().GetCompressor().Decompress(curChunk->mCompressedPosData, decompressedBuffer, &numBytes);
                MCORE_ASSERT(numBytes == requiredSize);

                // convert endian
                MCore::Endian::ConvertSignedInt16((int16*)decompressedBuffer, numBytes / 2);

                // recompress
                MCore::Free(curChunk->mCompressedPosData);
                GetWaveletCache().GetCompressor().Compress(decompressedBuffer, numBytes, &curChunk->mCompressedPosData, &curChunk->mCompressedPosNumBytes, false);
                curChunk->mCompressedPosNumBits = curChunk->mCompressedPosNumBytes * 8;
            }


    #ifndef EMFX_SCALE_DISABLED
            if (mNumScaleTracks > 0)
            {
                // decompress
                const uint32 requiredSize = GetWaveletCache().GetCompressor().CalcDecompressedSize(curChunk->mCompressedScaleData, curChunk->mCompressedScaleNumBytes);
                if (decompressedNumBytes < requiredSize)
                {
                    decompressedBuffer = (uint8*)MCore::Realloc(decompressedBuffer, requiredSize + 4096, EMFX_MEMCATEGORY_MOTIONS_WAVELETSKELETALMOTIONS);
                    decompressedNumBytes = requiredSize + 4096; // 4kb extra allocated already
                }

                uint32 numBytes = 0;
                GetWaveletCache().GetCompressor().Decompress(curChunk->mCompressedScaleData, decompressedBuffer, &numBytes);
                MCORE_ASSERT(numBytes == requiredSize);

                // convert endian
                MCore::Endian::ConvertSignedInt16((int16*)decompressedBuffer, numBytes / 2);

                // recompress
                MCore::Free(curChunk->mCompressedScaleData);
                GetWaveletCache().GetCompressor().Compress(decompressedBuffer, numBytes, &curChunk->mCompressedScaleData, &curChunk->mCompressedScaleNumBytes, false);
                curChunk->mCompressedScaleNumBits = curChunk->mCompressedScaleNumBytes * 8;
            }
    #endif  // EMFX_SCALE_DISABLED


            // morphs
            if (mNumMorphTracks > 0)
            {
                // decompress
                const uint32 requiredSize = GetWaveletCache().GetCompressor().CalcDecompressedSize(curChunk->mCompressedMorphData, curChunk->mCompressedMorphNumBytes);
                if (decompressedNumBytes < requiredSize)
                {
                    decompressedBuffer = (uint8*)MCore::Realloc(decompressedBuffer, requiredSize + 4096, EMFX_MEMCATEGORY_MOTIONS_WAVELETSKELETALMOTIONS);
                    decompressedNumBytes = requiredSize + 4096; // 4kb extra allocated already
                }

                uint32 numBytes = 0;
                GetWaveletCache().GetCompressor().Decompress(curChunk->mCompressedMorphData, decompressedBuffer, &numBytes);
                MCORE_ASSERT(numBytes == requiredSize);

                // convert endian
                MCore::Endian::ConvertSignedInt16((int16*)decompressedBuffer, numBytes / 2);

                // recompress
                MCore::Free(curChunk->mCompressedMorphData);
                GetWaveletCache().GetCompressor().Compress(decompressedBuffer, numBytes, &curChunk->mCompressedMorphData, &curChunk->mCompressedMorphNumBytes, false);
                curChunk->mCompressedMorphNumBits = curChunk->mCompressedMorphNumBytes * 8;
            }
        } // for all chunks

        // free the temp decompression buffer
        MCore::Free(decompressedBuffer);
    }


    void WaveletSkeletalMotion::SetSubMotionMapping(uint32 index, const Mapping& mapping)   { mSubMotionMap[index] = mapping; }
    void WaveletSkeletalMotion::ResizeMappingArray(uint32 numItems)
    {
        if (numItems == MCORE_INVALIDINDEX32)
        {
            mSubMotionMap.Resize(mSubMotions.GetLength());
        }
        else
        {
            mSubMotionMap.Resize(numItems);
        }
    }
    void WaveletSkeletalMotion::SetMorphSubMotionMapping(uint32 index, uint16 mapping)      { mMorphSubMotionMap[index] = mapping; }
    void WaveletSkeletalMotion::ResizeMorphMappingArray(uint32 numItems)
    {
        if (numItems == MCORE_INVALIDINDEX32)
        {
            mMorphSubMotionMap.Resize(mMorphSubMotions.GetLength());
        }
        else
        {
            mMorphSubMotionMap.Resize(numItems);
        }
    }
    void WaveletSkeletalMotion::SetWavelet(EWaveletType waveletType)                        { mWavelet = waveletType; }
    void WaveletSkeletalMotion::SetSampleSpacing(float spacing)                             { mSampleSpacing = spacing; }
    void WaveletSkeletalMotion::SetSecondsPerChunk(float secs)                              { mSecondsPerChunk = secs; }
    void WaveletSkeletalMotion::SetCompressedSize(uint32 numBytes)                          { mCompressedSize = numBytes; }
    void WaveletSkeletalMotion::SetOptimizedSize(uint32 numBytes)                           { mOptimizedSize = numBytes; }
    void WaveletSkeletalMotion::SetUncompressedSize(uint32 numBytes)                        { mUncompressedSize = numBytes; }
    void WaveletSkeletalMotion::SetNumPosTracks(uint32 numTracks)                           { mNumPosTracks = numTracks; }
    void WaveletSkeletalMotion::SetNumRotTracks(uint32 numTracks)                           { mNumRotTracks = numTracks; }
    void WaveletSkeletalMotion::SetNumMorphTracks(uint32 numTracks)                         { mNumMorphTracks = numTracks; }
    void WaveletSkeletalMotion::SetChunkOverhead(uint32 numBytes)                           { mChunkOverhead = numBytes; }
    void WaveletSkeletalMotion::SetSamplesPerChunk(uint32 numSamples)                       { mSamplesPerChunk = numSamples; }
    void WaveletSkeletalMotion::SetNumChunks(uint32 numChunks)                              { mChunks.Resize(numChunks); }
    void WaveletSkeletalMotion::SetChunk(uint32 index, Chunk* chunk)                        { mChunks[index] = chunk; }
    void WaveletSkeletalMotion::SetDecompressedRotNumBytes(uint32 numBytes)                 { mDecompressedRotNumBytes = numBytes; }
    void WaveletSkeletalMotion::SetDecompressedPosNumBytes(uint32 numBytes)                 { mDecompressedPosNumBytes = numBytes; }
    void WaveletSkeletalMotion::SetDecompressedMorphNumBytes(uint32 numBytes)               { mDecompressedMorphNumBytes = numBytes; }
    void WaveletSkeletalMotion::SetPosQuantFactor(float factor)                             { mPosQuantizeFactor = factor; }
    void WaveletSkeletalMotion::SetRotQuantFactor(float factor)                             { mRotQuantizeFactor = factor; }
    void WaveletSkeletalMotion::SetMorphQuantFactor(float factor)                           { mMorphQuantizeFactor = factor; }

#ifndef EMFX_SCALE_DISABLED
    void WaveletSkeletalMotion::SetNumScaleTracks(uint32 numTracks)                     { mNumScaleTracks = numTracks; }
    void WaveletSkeletalMotion::SetDecompressedScaleNumBytes(uint32 numBytes)           { mDecompressedScaleNumBytes = numBytes; }
    void WaveletSkeletalMotion::SetScaleQuantFactor(float factor)                       { mScaleQuantizeFactor = factor; }
#endif


    void WaveletSkeletalMotion::Scale(float scaleFactor)
    {
        mScale *= scaleFactor;

        // modify all submotions
        const uint32 numSubMotions = mSubMotions.GetLength();
        for (uint32 i = 0; i < numSubMotions; ++i)
        {
            SkeletalSubMotion* subMotion = mSubMotions[i];
            subMotion->SetBindPosePos(subMotion->GetBindPosePos() * scaleFactor);
            subMotion->SetPosePos(subMotion->GetPosePos() * scaleFactor);
        }

        // trigger the event
        GetEventManager().OnScaleMotionData(this, scaleFactor);

        // make sure we clear cached data, as this is now modified
        EMotionFX::GetWaveletCache().RemoveChunksForMotion(this);
    }


    float WaveletSkeletalMotion::GetScale() const
    {
        return mScale;
    }
}   // namespace EMotionFX
