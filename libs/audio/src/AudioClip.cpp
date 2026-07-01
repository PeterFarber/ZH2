#include "Hockey/Audio/AudioClip.hpp"

#include "Hockey/Assets/Assets/AudioAsset.hpp"

#include <miniaudio.h>

namespace Hockey {

Result<AudioClip> AudioClip::Decode(const AudioAsset& asset) {
    if (asset.encodedBytes.empty()) {
        return Result<AudioClip>::Fail("audio asset has no encoded bytes");
    }

    ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, 0);
    ma_decoder decoder{};
    ma_result result =
        ma_decoder_init_memory(asset.encodedBytes.data(), asset.encodedBytes.size(), &config, &decoder);
    if (result != MA_SUCCESS) {
        return Result<AudioClip>::Fail("failed to decode audio asset: " + asset.debugName);
    }

    struct DecoderGuard {
        ma_decoder* decoder = nullptr;
        ~DecoderGuard() {
            if (decoder != nullptr) {
                ma_decoder_uninit(decoder);
            }
        }
    } guard{&decoder};

    ma_uint64 frameCount = 0;
    result = ma_decoder_get_length_in_pcm_frames(&decoder, &frameCount);
    if (result != MA_SUCCESS || frameCount == 0 || decoder.outputChannels == 0 || decoder.outputSampleRate == 0) {
        return Result<AudioClip>::Fail("decoded audio contains no frames: " + asset.debugName);
    }

    AudioClip clip;
    clip.id = asset.id;
    clip.name = asset.debugName;
    clip.sampleRate = decoder.outputSampleRate;
    clip.channels = decoder.outputChannels;
    clip.samples.resize(static_cast<size_t>(frameCount) * static_cast<size_t>(clip.channels));

    ma_uint64 framesRead = 0;
    result = ma_decoder_read_pcm_frames(&decoder, clip.samples.data(), frameCount, &framesRead);
    if (result != MA_SUCCESS || framesRead == 0) {
        return Result<AudioClip>::Fail("failed to read decoded audio frames: " + asset.debugName);
    }

    clip.frameCount = framesRead;
    clip.samples.resize(static_cast<size_t>(framesRead) * static_cast<size_t>(clip.channels));
    clip.durationSeconds = static_cast<float>(static_cast<double>(framesRead) / static_cast<double>(clip.sampleRate));
    return Result<AudioClip>::Ok(std::move(clip));
}

} // namespace Hockey
