/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/*
 * This file is based on WME Lite.
 * http://dead-code.org/redir.php?target=wmelite
 * Copyright (c) 2011 Jan Nedoma
 */


#include "engines/wintermute/video/video_theora_player.h"
#include "engines/wintermute/base/base_game.h"
#include "engines/wintermute/base/base_file_manager.h"
#include "engines/wintermute/base/gfx/base_surface.h"
#include "engines/wintermute/base/gfx/base_image.h"
#include "engines/wintermute/base/gfx/base_renderer.h"
#include "engines/wintermute/base/sound/base_sound_manager.h"
#include "video/theora_decoder.h"
#include "engines/wintermute/wintermute.h"
#include "common/system.h"

namespace Wintermute {

IMPLEMENT_PERSISTENT(VideoTheoraPlayer, false)

//////////////////////////////////////////////////////////////////////////
VideoTheoraPlayer::VideoTheoraPlayer(BaseGame *inGame) : BaseClass(inGame) {
	SetDefaults();
}

//////////////////////////////////////////////////////////////////////////
void VideoTheoraPlayer::SetDefaults() {

	_filename = "";
	_startTime = 0;
	_looping = false;

	_freezeGame = false;
	_currentTime = 0;

	_state = THEORA_STATE_NONE;

	_videoFrameReady = false;
	_audioFrameReady = false;
	_videobufTime = 0;

	_playbackStarted = false;
	_dontDropFrames = false;

	_texture = nullptr;
	_alphaFilename = "";

	_frameRendered = false;

	_seekingKeyframe = false;
	_timeOffset = 0.0f;

	_posX = _posY = 0;
	_playbackType = VID_PLAY_CENTER;
	_playZoom = 0.0f;

	_savedState = THEORA_STATE_NONE;
	_savedPos = 0;
	_volume = 100;
	_theoraDecoder = nullptr;

	_subtitler = new VideoSubtitler(_gameRef);
	_foundSubtitles = false;
}

//////////////////////////////////////////////////////////////////////////
VideoTheoraPlayer::~VideoTheoraPlayer() {
	cleanup();
	delete _subtitler;
}

//////////////////////////////////////////////////////////////////////////
void VideoTheoraPlayer::cleanup() {
	if (_theoraDecoder) {
		_theoraDecoder->close();
	}
	delete _theoraDecoder;
	_theoraDecoder = nullptr;
	delete _texture;
	_texture = nullptr;
}

//////////////////////////////////////////////////////////////////////////
bool VideoTheoraPlayer::initialize(const Common::String &filename, const Common::String &subtitleFile) {
	cleanup();

	_filename = filename;

#if defined (USE_THEORADEC)
	// Load a file, but avoid having the File-manager handle the disposal of it.
	Common::SeekableReadStream *file = BaseFileManager::getEngineInstance()->openFile(filename, true, false);
	if (!file) {
		return STATUS_FAILED;
	}

	_theoraDecoder = new Video::TheoraDecoder();
	_theoraDecoder->loadStream(file);
#else
	warning("VideoTheoraPlayer::initialize - Theora support not compiled in, video will be skipped: %s", filename.c_str());
	return STATUS_FAILED;
#endif

	_foundSubtitles = _subtitler->loadSubtitles(_filename, subtitleFile);

	if (!_theoraDecoder->isVideoLoaded()) {
		return STATUS_FAILED;
	}

	_state = THEORA_STATE_PAUSED;

	// Additional setup.
	_texture = _gameRef->_renderer->createSurface();
	_texture->create(_theoraDecoder->getWidth(), _theoraDecoder->getHeight());
	_state = THEORA_STATE_PLAYING;
	_playZoom = 100;

	return STATUS_OK;
}


//////////////////////////////////////////////////////////////////////////
bool VideoTheoraPlayer::resetStream() {
	// HACK: Just reopen the same file again.
	if (_theoraDecoder) {
		_theoraDecoder->close();
	}
	delete _theoraDecoder;
	_theoraDecoder = nullptr;

#if defined (USE_THEORADEC)
	// Load a file, but avoid having the File-manager handle the disposal of it.
	Common::SeekableReadStream *file = BaseFileManager::getEngineInstance()->openFile(_filename, true, false);
	if (!file) {
		return STATUS_FAILED;
	}

	_theoraDecoder = new Video::TheoraDecoder();
	_theoraDecoder->loadStream(file);
#else
	return STATUS_FAILED;
#endif

	if (!_theoraDecoder->isVideoLoaded()) {
		return STATUS_FAILED;
	}

	return play(_playbackType, _posX, _posY, false, false, _looping, 0, _playZoom);
	// End of hack.
#if 0 // Stubbed for now, as theora isn't seekable
	if (_sound) {
		_sound->Stop();
	}

	m_TimeOffset = 0.0f;
	Initialize(m_Filename);
	Play(m_PlaybackType, m_PosX, m_PosY, false, false, m_Looping, 0, m_PlayZoom);
#endif
	return STATUS_OK;
}

//////////////////////////////////////////////////////////////////////////
bool VideoTheoraPlayer::play(TVideoPlayback type, int x, int y, bool freezeGame, bool freezeMusic, bool looping, uint32 startTime, float forceZoom, int volume) {
	if (forceZoom < 0.0f) {
		forceZoom = 100.0f;
	}
	if (volume < 0) {
		_volume = _gameRef->_soundMgr->getVolumePercent(Audio::Mixer::kSFXSoundType);
	} else {
		_volume = volume;
	}

	_freezeGame = freezeGame;

	if (!_playbackStarted && _freezeGame) {
		_gameRef->freeze(freezeMusic);
	}

	_playbackStarted = false;
	float width, height;
	if (_theoraDecoder) {
		_state = THEORA_STATE_PLAYING;
		_looping = looping;
		_playbackType = type;
		if (_subtitler && _foundSubtitles && _gameRef->_subtitles) {
			_subtitler->update(_theoraDecoder->getFrameCount());
			_subtitler->display();
		}
		_startTime = startTime;
		_volume = volume;
		_posX = x;
		_posY = y;
		_playZoom = forceZoom;

		width = (float)_theoraDecoder->getWidth();
		height = (float)_theoraDecoder->getHeight();
	} else {
		width = (float)_gameRef->_renderer->getWidth();
		height = (float)_gameRef->_renderer->getHeight();
	}

	switch (type) {
	case VID_PLAY_POS:
		_playZoom = forceZoom;
		_posX = x;
		_posY = y;
		break;

	case VID_PLAY_STRETCH: {
		float zoomX = (float)((float)_gameRef->_renderer->getWidth() / width * 100);
		float zoomY = (float)((float)_gameRef->_renderer->getHeight() / height * 100);
		_playZoom = MIN(zoomX, zoomY);
		_posX = (int)((_gameRef->_renderer->getWidth() - width * (_playZoom / 100)) / 2);
		_posY = (int)((_gameRef->_renderer->getHeight() - height * (_playZoom / 100)) / 2);
	}
	break;

	case VID_PLAY_CENTER:
		_playZoom = 100.0f;
		_posX = (int)((_gameRef->_renderer->getWidth() - width) / 2);
		_posY = (int)((_gameRef->_renderer->getHeight() - height) / 2);
		break;

	default:
		break;
	}

	if (_theoraDecoder)
		_theoraDecoder->start();

	return STATUS_OK;
#if 0 // Stubbed for now as theora isn't seekable
	if (StartTime) SeekToTime(StartTime);

	update();
#endif
	return STATUS_FAILED;
}

//////////////////////////////////////////////////////////////////////////
bool VideoTheoraPlayer::stop() {
	_theoraDecoder->close();
	_state = THEORA_STATE_FINISHED;
	if (_freezeGame) {
		_gameRef->unfreeze();
	}

	return STATUS_OK;
}

//////////////////////////////////////////////////////////////////////////
bool VideoTheoraPlayer::update() {
	_currentTime = _freezeGame ? _gameRef->getLiveTimer()->getTime() : _gameRef->getTimer()->getTime();

	if (!isPlaying()) {
		return STATUS_OK;
	}

	if (_playbackStarted /*&& m_Sound && !m_Sound->IsPlaying()*/) {
		return STATUS_OK;
	}

	if (_playbackStarted && !_freezeGame && _gameRef->_state == GAME_FROZEN) {
		return STATUS_OK;
	}

	if (_theoraDecoder) {
		if (_subtitler && _foundSubtitles && _gameRef->_subtitles) {
			_subtitler->update(_theoraDecoder->getCurFrame());
		}

		if (_theoraDecoder->endOfVideo() && _looping) {
			_theoraDecoder->rewind();
			// HACK: Just reinitialize the same video again
			return resetStream();
		} else if (_theoraDecoder->endOfVideo() && !_looping) {
			debugC(kWintermuteDebugLog, "Finished movie %s", _filename.c_str());
			_state = THEORA_STATE_FINISHED;
			_playbackStarted = false;
			if (_freezeGame) {
				_gameRef->unfreeze();
			}
		}
		if (_state == THEORA_STATE_PLAYING) {
			if (!_theoraDecoder->endOfVideo() && _theoraDecoder->getTimeToNextFrame() == 0) {
				const Graphics::Surface *decodedFrame = _theoraDecoder->decodeNextFrame();
				if (decodedFrame && _texture) {
					writeVideo(decodedFrame);
				}
			}
			return STATUS_OK;
		}
	}
	// Skip the busy-loop?
	if ((!_texture || !_videoFrameReady) && _theoraDecoder && !_theoraDecoder->endOfVideo()) {
		// end playback
		if (!_looping) {
			_state = THEORA_STATE_FINISHED;
			if (_freezeGame) {
				_gameRef->unfreeze();
			}
			return STATUS_OK;
		} else {
			resetStream();
			return STATUS_OK;
		}
	}

	return STATUS_OK;
}

//////////////////////////////////////////////////////////////////////////
uint32 VideoTheoraPlayer::getMovieTime() const {
	if (!_playbackStarted) {
		return 0;
	} else {
		return _theoraDecoder->getTime();
	}
}

//////////////////////////////////////////////////////////////////////////
bool VideoTheoraPlayer::writeVideo(const Graphics::Surface *decodedFrame) {
	if (!_texture) {
		return STATUS_FAILED;
	}

	_texture->putSurface(*decodedFrame, false);
	//RenderFrame(_texture, &yuv);

	_videoFrameReady = true;
	return STATUS_OK;
}

//////////////////////////////////////////////////////////////////////////
bool VideoTheoraPlayer::display(uint32 alpha) {
	Rect32 rc;
	bool res;

	if (_texture && _videoFrameReady) {
		rc.setRect(0, 0, _texture->getWidth(), _texture->getHeight());
		if (_playZoom == 100.0f) {
			res = _texture->displayTrans(_posX, _posY, rc, alpha);
		} else {
			res = _texture->displayTransZoom(_posX, _posY, rc, _playZoom, _playZoom, alpha);
		}
	} else {
		res = STATUS_FAILED;
	}

	if (_subtitler && _foundSubtitles && _gameRef->_subtitles) {
		_subtitler->display();
	}
	return res;
}

//////////////////////////////////////////////////////////////////////////
bool VideoTheoraPlayer::setAlphaImage(const Common::String &filename) {
	assert(_texture);
	if (filename == "" || !_texture || DID_FAIL(_texture->setAlphaImage(filename))) {
		_alphaFilename = "";
		return STATUS_FAILED;
	}

	if (_alphaFilename != filename) {
		_alphaFilename = filename;
	}
	return STATUS_OK;
}

//////////////////////////////////////////////////////////////////////////
inline int intlog(int num) {
	int r = 0;
	while (num > 0) {
		num = num / 2;
		r = r + 1;
	}

	return r;
}

//////////////////////////////////////////////////////////////////////////
bool VideoTheoraPlayer::seekToTime(uint32 time) {
	error("VideoTheoraPlayer::SeekToTime(%d) - not supported", time);
	return STATUS_OK;
}

//////////////////////////////////////////////////////////////////////////
bool VideoTheoraPlayer::pause() {
	if (_state == THEORA_STATE_PLAYING) {
		_state = THEORA_STATE_PAUSED;
		_theoraDecoder->pauseVideo(true);
		return STATUS_OK;
	} else {
		return STATUS_FAILED;
	}
}

//////////////////////////////////////////////////////////////////////////
bool VideoTheoraPlayer::resume() {
	if (_state == THEORA_STATE_PAUSED) {
		_state = THEORA_STATE_PLAYING;
		_theoraDecoder->pauseVideo(false);
		return STATUS_OK;
	} else {
		return STATUS_FAILED;
	}
}

//////////////////////////////////////////////////////////////////////////
bool VideoTheoraPlayer::persist(BasePersistenceManager *persistMgr) {
	//BaseClass::persist(persistMgr);

	if (persistMgr->getIsSaving()) {
		_savedPos = getMovieTime() * 1000;
		_savedState = _state;
	} else {
		SetDefaults();
	}

	persistMgr->transferPtr(TMEMBER_PTR(_gameRef));
	persistMgr->transferUint32(TMEMBER(_savedPos));
	persistMgr->transferSint32(TMEMBER(_savedState));
	persistMgr->transferString(TMEMBER(_filename));
	persistMgr->transferString(TMEMBER(_alphaFilename));
	persistMgr->transferSint32(TMEMBER(_posX));
	persistMgr->transferSint32(TMEMBER(_posY));
	persistMgr->transferFloat(TMEMBER(_playZoom));
	persistMgr->transferSint32(TMEMBER_INT(_playbackType));
	persistMgr->transferBool(TMEMBER(_looping));
	persistMgr->transferSint32(TMEMBER(_volume));

	if (!persistMgr->getIsSaving() && (_savedState != THEORA_STATE_NONE)) {
		initializeSimple();
	}

	return STATUS_OK;
}

//////////////////////////////////////////////////////////////////////////
bool VideoTheoraPlayer::initializeSimple() {
	if (DID_SUCCEED(initialize(_filename))) {
		if (_alphaFilename != "") {
			setAlphaImage(_alphaFilename);
		}
		play(_playbackType, _posX, _posY, false, false, _looping, _savedPos, _playZoom);
	} else {
		_state = THEORA_STATE_FINISHED;
	}

	return STATUS_OK;
}

//////////////////////////////////////////////////////////////////////////
BaseSurface *VideoTheoraPlayer::getTexture() const {
	return _texture;
}

} // End of namespace Wintermute
