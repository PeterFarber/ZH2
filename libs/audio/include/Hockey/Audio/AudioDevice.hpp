#pragma once

#include <memory>

#include "Hockey/Audio/AudioBus.hpp"
#include "Hockey/Audio/AudioSettings.hpp"
#include "Hockey/Core/Result.hpp"

namespace Hockey {

struct AudioClip;
class AudioMixer;

class AudioDevice {
public:
    AudioDevice();
    ~AudioDevice();

    AudioDevice(const AudioDevice&) = delete;
    AudioDevice& operator=(const AudioDevice&) = delete;

    Status Init(const AudioSettings& settings);
    void Shutdown();
    void Update();
    bool IsRunning() const;
    bool IsNullBackend() const;
    Status PlayOneShot(std::shared_ptr<AudioClip> clip, AudioBusType bus, float volume, const AudioMixer& mixer);
    void StopAll();

private:
    struct Impl;
    std::unique_ptr<Impl> m_Impl;
};

} // namespace Hockey
