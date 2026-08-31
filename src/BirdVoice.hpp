#pragma once
#include <rack.hpp>
#include <cmath>
#include <algorithm>

namespace rack {

static constexpr float T60_MULTIPLIER = 6.907755278982137f; // -ln(0.001)
static constexpr float FREQ_MULTIPLIER = 7000.0f;
static constexpr float FREQ_OFFSET = 300.0f;
static constexpr float MAX_ATTACK_DECAY_TIME = 0.9f;
static constexpr float ENVELOPE_FREQ_MULTIPLIER = 3000.0f;

struct BirdParams {
	float ifrq;
	float atk;
	float dcy;
	float fmod1;
	float atkf1;
	float dcyf1;
	float fmod2;
	float atkf2;
	float dcyf2;
	float amod1;
	float atka1;
	float dcya1;
	float amod2;
	float atka2;
	float dcya2;

	BirdParams(
		float ifrq = 0.55102f, float atk = 0.591837f, float dcy = 0.187755f,
		float fmod1 = 0.0716327f, float atkf1 = 0.0204082f, float dcyf1 = 0.346939f,
		float fmod2 = 0.0204082f, float atkf2 = 0.55102f, float dcyf2 = 0.122449f,
		float amod1 = 0.632653f, float atka1 = 1.0f, float dcya1 = 0.612245f,
		float amod2 = 0.346939f, float atka2 = 0.816327f, float dcya2 = 0.653061f
	) : ifrq(ifrq), atk(atk), dcy(dcy),
	    fmod1(fmod1), atkf1(atkf1), dcyf1(dcyf1),
	    fmod2(fmod2), atkf2(atkf2), dcyf2(dcyf2),
	    amod1(amod1), atka1(atka1), dcya1(dcya1),
	    amod2(amod2), atka2(atka2), dcya2(dcya2) {}
};

inline const BirdParams& getBirdPreset(int index) {
	static const BirdParams presets[8] = {
		// 0: Lesser-Spotted Grinchwarbler
		{0.55102f, 0.591837f, 0.187755f, 0.0716327f, 0.0204082f, 0.346939f, 0.0204082f, 0.55102f, 0.122449f, 0.632653f, 1.0f, 0.612245f, 0.346939f, 0.816327f, 0.653061f},
		// 1: Speckled-Throated Spew
		{0.183673f, 0.591837f, 0.387755f, 0.0104082f, 0.530612f, 0.346939f, 0.244898f, 0.55102f, 0.122449f, 0.387755f, 1.0f, 0.612245f, 0.346939f, 0.816327f, 0.653061f},
		// 2: Triple-Tailed Tree-Troubler
		{0.387755f, 0.0204082f, 0.204082f, 0.367347f, 0.571429f, 0.734694f, 0.918367f, 1.0f, 0.77551f, 0.571429f, 0.367347f, 0.22449f, 0.0204082f, 0.183673f, 0.44898f},
		// 3: Long-Toed Mudhopper
		{0.163265f, 0.22449f, 0.183673f, 0.00306122f, 0.122449f, 1.0f, 0.0612245f, 1.0f, 0.77551f, 0.979592f, 0.204082f, 0.734694f, 1.0f, 0.142857f, 0.612245f},
		// 4: Yellow-Yiffled Yaffle
		{0.0204082f, 0.367347f, 0.183673f, 0.0612245f, 0.0f, 1.0f, 0.285714f, 0.22449f, 0.489796f, 0.367347f, 0.387755f, 0.734694f, 0.204082f, 0.428571f, 0.142857f},
		// 5: Pointy-Beaked Beetlefiend
		{0.388571f, 0.204082f, 0.309796f, 0.0204082f, 0.795918f, 0.591837f, 0.285714f, 0.22449f, 0.489796f, 0.204082f, 0.836735f, 0.734694f, 0.77551f, 0.428571f, 0.142857f},
		// 6: African Boojuboolubala
		{0.306122f, 0.959184f, 0.0408163f, 1.0f, 0.0f, 0.591837f, 0.285714f, 0.22449f, 0.489796f, 0.204082f, 0.836735f, 0.734694f, 0.77551f, 0.428571f, 0.142857f},
		// 7: Common Muckoink
		{0.0204082f, 0.8f, 0.0816327f, 0.0204082f, 0.001f, 0.99f, 0.0204082f, 0.01f, 1.0f, 1.0f, 0.142857f, 0.734694f, 1.0f, 0.0612245f, 0.530612f}
	};
	int idx = math::clamp(index, 0, 7);
	return presets[idx];
}

inline const char* getBirdPresetName(int index) {
	static const char* names[8] = {
		"Lesser-Spotted Grinchwarbler",
		"Speckled-Throated Spew",
		"Triple-Tailed Tree-Troubler",
		"Long-Toed Mudhopper",
		"Yellow-Yiffled Yaffle",
		"Pointy-Beaked Beetlefiend",
		"African Boojuboolubala",
		"Common Muckoink"
	};
	int idx = math::clamp(index, 0, 7);
	return names[idx];
}

class ExponentialEAD {
public:
	enum Stage {
		IDLE,
		ATTACK,
		DECAY
	};

	Stage stage = IDLE;
	float minValue = 0.0f;
	float maxValue = 1.0f;
	float attackTime = 0.1f;
	float decayTime = 0.1f;

	float timer = 0.0f;
	float decayTimer = 0.0f;
	float attackEndVal = 0.0f;
	float currentValue = 0.0f;

	void trigger(float minVal, float maxVal, float atk, float dcy) {
		minValue = minVal;
		maxValue = maxVal;
		attackTime = std::max(atk, 1e-4f);
		decayTime = std::max(dcy, 1e-4f);

		stage = ATTACK;
		timer = 0.0f;
		decayTimer = 0.0f;
		attackEndVal = 0.0f;
		currentValue = minValue;
	}

	void stop() {
		stage = IDLE;
		currentValue = minValue;
	}

	bool isActive() const {
		return stage != IDLE;
	}

	float process(float dt) {
		if (stage == ATTACK) {
			timer += dt;
			float tau = attackTime / T60_MULTIPLIER;
			currentValue = minValue + (maxValue - minValue) * (1.0f - std::exp(-timer / tau));

			if (timer >= attackTime) {
				stage = DECAY;
				attackEndVal = currentValue;
				decayTimer = 0.0f;
			}
		} else if (stage == DECAY) {
			decayTimer += dt;
			float tau = decayTime / T60_MULTIPLIER;
			currentValue = minValue + (attackEndVal - minValue) * std::exp(-decayTimer / tau);

			if (decayTimer >= 4.0f * decayTime || std::abs(currentValue - minValue) < 1e-5f) {
				stage = IDLE;
				currentValue = minValue;
			}
		} else {
			currentValue = minValue;
		}
		return currentValue;
	}

	float getValue() const {
		return currentValue;
	}
};

class BirdVoice {
public:
	ExponentialEAD mainEnv;
	ExponentialEAD fmFreqEnv;
	ExponentialEAD amFreqEnv;
	ExponentialEAD fmGainEnv;
	ExponentialEAD amGainEnv;

	float carrierPhase = 0.0f;
	float fmPhase = 0.0f;
	float amPhase = 0.0f;

	float delayTimer = 0.0f;
	bool pendingChirp = false;
	bool active = false;

	BirdParams activeParams;
	float basePitchVoct = 0.0f;
	float pan = 0.0f; // -1.0 to +1.0

	void trigger(const BirdParams& params, float delaySec = 0.0f, float voct = 0.0f, float panPos = 0.0f) {
		activeParams = params;
		basePitchVoct = voct;
		pan = math::clamp(panPos, -1.0f, 1.0f);

		if (delaySec > 0.0f) {
			delayTimer = delaySec;
			pendingChirp = true;
		} else {
			delayTimer = 0.0f;
			pendingChirp = false;
			startChirp();
		}
	}

	void startChirp() {
		pendingChirp = false;
		active = true;

		// Main amplitude envelope (0.0 to 1.0)
		mainEnv.trigger(0.0f, 1.0f,
			MAX_ATTACK_DECAY_TIME * activeParams.atk,
			MAX_ATTACK_DECAY_TIME * activeParams.dcy);

		// FM frequency envelope (0.0 to 3000 * fmod1)
		fmFreqEnv.trigger(0.0f, ENVELOPE_FREQ_MULTIPLIER * activeParams.fmod1,
			MAX_ATTACK_DECAY_TIME * activeParams.atkf1,
			MAX_ATTACK_DECAY_TIME * activeParams.dcyf1);

		// AM frequency envelope (0.0 to 3000 * fmod2)
		amFreqEnv.trigger(0.0f, ENVELOPE_FREQ_MULTIPLIER * activeParams.fmod2,
			MAX_ATTACK_DECAY_TIME * activeParams.atkf2,
			MAX_ATTACK_DECAY_TIME * activeParams.dcyf2);

		// FM gain envelope (0.0 to amod1)
		fmGainEnv.trigger(0.0f, activeParams.amod1,
			MAX_ATTACK_DECAY_TIME * activeParams.atka1,
			MAX_ATTACK_DECAY_TIME * activeParams.dcya1);

		// AM gain envelope (0.0 to -amod2)
		amGainEnv.trigger(0.0f, -activeParams.amod2,
			MAX_ATTACK_DECAY_TIME * activeParams.atka2,
			MAX_ATTACK_DECAY_TIME * activeParams.dcya2);
	}

	bool isPlaying() const {
		return pendingChirp || mainEnv.isActive();
	}

	float getEnvelope() const {
		return mainEnv.getValue();
	}

	void process(float dt, float& outL, float& outR) {
		outL = 0.0f;
		outR = 0.0f;

		if (pendingChirp) {
			delayTimer -= dt;
			if (delayTimer <= 0.0f) {
				startChirp();
			} else {
				return;
			}
		}

		if (!mainEnv.isActive()) {
			active = false;
			return;
		}

		// Advance envelopes
		float gMain = mainEnv.process(dt);
		float fFm = fmFreqEnv.process(dt);
		float fAm = amFreqEnv.process(dt);
		float gFm = fmGainEnv.process(dt);
		float gAm = amGainEnv.process(dt);

		// FM Modulator Oscillator
		fmPhase += fFm * dt;
		if (fmPhase >= 1.0f) fmPhase -= std::floor(fmPhase);
		float sFm = std::sin(2.0f * M_PI * fmPhase);

		// Carrier Base Frequency with V/Oct
		float f0 = FREQ_OFFSET + FREQ_MULTIPLIER * math::clamp(activeParams.ifrq, 0.0f, 1.0f);
		if (basePitchVoct != 0.0f) {
			f0 *= std::pow(2.0f, basePitchVoct);
		}
		f0 = math::clamp(f0, 10.0f, 20000.0f);

		// FM Modulator applies frequency deviation: F0 * (1 + gFm * sFm)
		float carrierFreq = f0 * (1.0f + gFm * sFm);
		carrierFreq = std::max(0.0f, carrierFreq);

		// Carrier Oscillator
		carrierPhase += carrierFreq * dt;
		if (carrierPhase >= 1.0f) carrierPhase -= std::floor(carrierPhase);
		float sCarrier = std::sin(2.0f * M_PI * carrierPhase);

		// AM Modulator Oscillator
		amPhase += fAm * dt;
		if (amPhase >= 1.0f) amPhase -= std::floor(amPhase);
		float sAm = std::sin(2.0f * M_PI * amPhase);

		// AM synthesis: carrier * (1 + gAm * sAm)
		// gAm goes from 0.0 down to -amod2
		float amGain = 1.0f + gAm * sAm;
		float sAmSignal = sCarrier * amGain;

		// Main amplitude scaling (VCV Rack audio level: +/- 5V)
		float outMono = sAmSignal * gMain * 5.0f;

		// Equal power stereo panning
		float panNorm = (pan + 1.0f) * 0.5f; // 0.0 (Left) to 1.0 (Right)
		float leftGain = std::cos(panNorm * (M_PI * 0.5f));
		float rightGain = std::sin(panNorm * (M_PI * 0.5f));

		outL = outMono * leftGain;
		outR = outMono * rightGain;
	}
};

} // namespace rack
