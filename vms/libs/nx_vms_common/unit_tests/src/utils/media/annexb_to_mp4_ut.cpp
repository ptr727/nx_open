// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include <gtest/gtest.h>

#include <utils/media/annexb_to_mp4.h>
#include <nx/media/ffmpeg_helper.h>

TEST(AnnexbToMp4, EmptyCodecParameters)
{
    uint8_t frameData[] = {
        0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0x00, 0x29, 0xe3, 0x50, 0x14, 0x07, 0xb6, 0x02, 0xdc,
        0x04, 0x04, 0x06, 0x90, 0x78, 0x91, 0x15, 0x00, 0x00, 0x00, 0x01, 0x68, 0xce, 0x3c, 0x80,
        0x00, 0x00, 0x00, 0x01, 0x65, 0x88, 0x84, 0x00, 0x00, 0x0c, 0x21, 0x18, 0xa0, 0x00, 0x67,
        0xf9, 0x39, 0x39, 0x39, 0x39, 0x38, 0x8f, 0xd1, 0x1e, 0xa4, 0xe2, 0x35, 0xe9, 0x38 };

    nx::media::AnnexbToMp4 converter;

    QnWritableCompressedVideoData frame(sizeof(frameData));
    frame.compressionType = AV_CODEC_ID_H264;
    frame.flags |= QnAbstractMediaData::MediaFlags_AVKey;
    frame.m_data.uncheckedWrite((char*)frameData, sizeof(frameData));
    auto codecParameters = QnFfmpegHelper::createVideoCodecParametersAnnexB(&frame);
    ASSERT_TRUE(codecParameters != nullptr);
    frame.context = codecParameters;
    converter.process(&frame);
    frame.context = nullptr;
    converter.process(&frame);
}

TEST(AnnexbToMp4, NoSpsPpps)
{
    uint8_t frameDataSpsPps[] = {
        0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0x00, 0x29, 0xe3, 0x50, 0x14, 0x07, 0xb6, 0x02, 0xdc,
        0x04, 0x04, 0x06, 0x90, 0x78, 0x91, 0x15, 0x00, 0x00, 0x00, 0x01, 0x68, 0xce, 0x3c, 0x80,
        0x00, 0x00, 0x00, 0x01, 0x65, 0x88, 0x84, 0x00, 0x00, 0x0c, 0x21, 0x18, 0xa0, 0x00, 0x67,
        0xf9, 0x39, 0x39, 0x39, 0x39, 0x38, 0x8f, 0xd1, 0x1e, 0xa4, 0xe2, 0x35, 0xe9, 0x38 };

    uint8_t frameDataNoSpsPps[] = {
        0x00, 0x00, 0x00, 0x01, 0x65, 0x88, 0x84, 0x00, 0x00, 0x0c, 0x21, 0x18, 0xa0, 0x00, 0x67,
        0xf9, 0x39, 0x39, 0x39, 0x39, 0x38, 0x8f, 0xd1, 0x1e, 0xa4, 0xe2, 0x35, 0xe9, 0x38 };

    nx::media::AnnexbToMp4 converter;

    QnWritableCompressedVideoData frame(sizeof(frameDataSpsPps));
    frame.compressionType = AV_CODEC_ID_H264;
    frame.flags |= QnAbstractMediaData::MediaFlags_AVKey;
    frame.m_data.uncheckedWrite((char*)frameDataSpsPps, sizeof(frameDataSpsPps));
    auto codecParameters = QnFfmpegHelper::createVideoCodecParametersAnnexB(&frame);
    ASSERT_TRUE(codecParameters != nullptr);
    frame.context = codecParameters;
    converter.process(&frame);

    frame.m_data.clear();
    frame.m_data.write((char*)frameDataNoSpsPps, sizeof(frameDataNoSpsPps));
    converter.process(&frame);
}
