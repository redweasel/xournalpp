#include <control/Control.h>
#include "AudioPlayer.h"

AudioPlayer::AudioPlayer(Control* control, Settings* settings) : control(control), settings(settings)
{
	this->audioQueue = new AudioQueue<float>();
	this->portAudioConsumer = new PortAudioConsumer(this, this->audioQueue);
	this->vorbisProducer = new VorbisProducer(this->audioQueue);
}

AudioPlayer::~AudioPlayer()
{
	this->stop();

	delete this->portAudioConsumer;
	this->portAudioConsumer = nullptr;

	delete this->vorbisProducer;
	this->vorbisProducer = nullptr;

	delete this->audioQueue;
	this->audioQueue = nullptr;
}

auto AudioPlayer::start(string filename, unsigned int timestamp) -> bool
{
	// Start the producer for reading the data
	bool status = this->vorbisProducer->start(std::move(filename), timestamp);

	// Start playing
	if (status)
	{
		status = status && this->play();
	}

	return status;
}

auto AudioPlayer::isPlaying() -> bool
{
	return this->portAudioConsumer->isPlaying();
}

void AudioPlayer::pause()
{
	if (!this->portAudioConsumer->isPlaying())
	{
		return;
	}

	// Stop playing audio
	this->portAudioConsumer->stopPlaying();
}

auto AudioPlayer::play() -> bool
{
	if (this->portAudioConsumer->isPlaying())
	{
		return false;
	}

	return this->portAudioConsumer->startPlaying();
}

void AudioPlayer::disableAudioPlaybackButtons()
{
	if (this->audioQueue->hasStreamEnded())
	{
		this->control->getWindow()->disableAudioPlaybackButtons();
	}
}

void AudioPlayer::stop()
{
	// Stop playing audio
	this->portAudioConsumer->stopPlaying();

	this->audioQueue->signalEndOfStream();

	// Abort libsox
	this->vorbisProducer->abort();

	// Reset the queue for the next playback
	this->audioQueue->reset();
}

void AudioPlayer::seek(int seconds)
{
	// set seek flag here in vorbisProducer
	this->vorbisProducer->seek(seconds);
}

auto AudioPlayer::getOutputDevices() -> vector<DeviceInfo>
{
	std::list<DeviceInfo> deviceList = this->portAudioConsumer->getOutputDevices();
	return vector<DeviceInfo>{std::make_move_iterator(std::begin(deviceList)),
							  std::make_move_iterator(std::end(deviceList))};
}

auto AudioPlayer::getSettings() -> Settings*
{
	return this->settings;
}
