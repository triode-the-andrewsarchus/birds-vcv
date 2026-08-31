#include "plugin.hpp"
#include "BirdVoice.hpp"

struct Birds : Module {
	enum ParamId {
		PRESET_PARAM,
		CHIRP_PARAM,
		REPEAT_PARAM,
		RATE_PARAM,

		// 15 Bird synthesis params
		IFRQ_PARAM,
		ATK_PARAM,
		DCY_PARAM,
		FMOD1_PARAM,
		ATKF1_PARAM,
		DCYF1_PARAM,
		FMOD2_PARAM,
		ATKF2_PARAM,
		DCYF2_PARAM,
		AMOD1_PARAM,
		ATKA1_PARAM,
		DCYA1_PARAM,
		AMOD2_PARAM,
		ATKA2_PARAM,
		DCYA2_PARAM,

		// Flock & Generative params
		VOICES_PARAM,
		PITCH_JITTER_PARAM,
		TIME_SPREAD_PARAM,
		PAN_SPREAD_PARAM,
		TIME_JITTER_PARAM,
		ENV_SPREAD_PARAM,

		PARAMS_LEN
	};

	enum InputId {
		TRIG_INPUT,
		VOCT_INPUT,
		FMCV_INPUT,
		AMCV_INPUT,
		RATECV_INPUT,
		SPREADCV_INPUT,
		INPUTS_LEN
	};

	enum OutputId {
		ENV_OUTPUT,
		OUTL_OUTPUT,
		OUTR_OUTPUT,
		OUTPUTS_LEN
	};

	enum LightId {
		CHIRP_LIGHT,
		REPEAT_LIGHT,
		MODIFIED_LIGHT,
		LIGHTS_LEN
	};

	static constexpr int MAX_VOICES = 8;
	BirdVoice voices[MAX_VOICES];

	dsp::SchmittTrigger trigTrigger;
	dsp::SchmittTrigger buttonTrigger;
	dsp::PulseGenerator chirpLightPulse;

	int lastPreset = -1;
	float repeatTimer = 0.0f;
	float nextRepeatInterval = 0.5f;

	Birds() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

		configParam(PRESET_PARAM, 0.f, 7.f, 0.f, "Preset (Bird Species)");
		configButton(CHIRP_PARAM, "Manual Chirp Trigger");
		configSwitch(REPEAT_PARAM, 0.f, 1.f, 0.f, "Auto Repeat Chirp", {"Off", "On"});
		configParam(RATE_PARAM, 0.1f, 10.0f, 1.5f, "Repeat Rate", " Hz");

		// 15 synthesis params
		configParam(IFRQ_PARAM, 0.f, 1.f, 0.55102f, "Base Frequency (IFRQ)", " normalized");
		configParam(ATK_PARAM, 0.001f, 1.f, 0.591837f, "Main Amp Attack (ATK)", " normalized");
		configParam(DCY_PARAM, 0.001f, 1.f, 0.187755f, "Main Amp Decay (DCY)", " normalized");

		configParam(FMOD1_PARAM, 0.f, 1.f, 0.0716327f, "FM Max Frequency (FMOD 1)", " normalized");
		configParam(ATKF1_PARAM, 0.001f, 1.f, 0.0204082f, "FM Freq Attack (ATKF 1)", " normalized");
		configParam(DCYF1_PARAM, 0.001f, 1.f, 0.346939f, "FM Freq Decay (DCYF 1)", " normalized");

		configParam(FMOD2_PARAM, 0.f, 1.f, 0.0204082f, "AM Max Frequency (FMOD 2)", " normalized");
		configParam(ATKF2_PARAM, 0.001f, 1.f, 0.55102f, "AM Freq Attack (ATKF 2)", " normalized");
		configParam(DCYF2_PARAM, 0.001f, 1.f, 0.122449f, "AM Freq Decay (DCYF 2)", " normalized");

		configParam(AMOD1_PARAM, 0.f, 1.f, 0.632653f, "FM Modulation Depth (AMOD 1)", " normalized");
		configParam(ATKA1_PARAM, 0.001f, 1.f, 1.0f, "FM Gain Attack (ATKA 1)", " normalized");
		configParam(DCYA1_PARAM, 0.001f, 1.f, 0.612245f, "FM Gain Decay (DCYA 1)", " normalized");

		configParam(AMOD2_PARAM, 0.f, 1.f, 0.346939f, "AM Modulation Depth (AMOD 2)", " normalized");
		configParam(ATKA2_PARAM, 0.001f, 1.f, 0.816327f, "AM Gain Attack (ATKA 2)", " normalized");
		configParam(DCYA2_PARAM, 0.001f, 1.f, 0.653061f, "AM Gain Decay (DCYA 2)", " normalized");

		// Flock params
		configParam(VOICES_PARAM, 1.f, 8.f, 1.f, "Flock Voices", " voices");
		configParam(PITCH_JITTER_PARAM, 0.f, 1.f, 0.15f, "Pitch Jitter", " normalized");
		configParam(TIME_SPREAD_PARAM, 0.f, 1.f, 0.3f, "Flock Time Spread", " s");
		configParam(PAN_SPREAD_PARAM, 0.f, 1.f, 0.5f, "Flock Stereo Pan Spread", " normalized");
		configParam(TIME_JITTER_PARAM, 0.f, 1.f, 0.2f, "Repeat Time Jitter", " normalized");
		configParam(ENV_SPREAD_PARAM, 0.f, 1.f, 0.2f, "Flock Envelope Spread", " normalized");

		// Inputs & Outputs
		configInput(TRIG_INPUT, "Chirp Trigger");
		configInput(VOCT_INPUT, "Pitch (1V/Oct)");
		configInput(FMCV_INPUT, "FM Depth CV");
		configInput(AMCV_INPUT, "AM Depth CV");
		configInput(RATECV_INPUT, "Repeat Rate CV");
		configInput(SPREADCV_INPUT, "Flock Spread CV");

		configOutput(ENV_OUTPUT, "Envelope (0-10V)");
		configOutput(OUTL_OUTPUT, "Left Audio");
		configOutput(OUTR_OUTPUT, "Right Audio");

		// Apply initial preset
		applyPreset(0);
	}

	void applyPreset(int index) {
		const BirdParams& p = getBirdPreset(index);
		params[IFRQ_PARAM].setValue(p.ifrq);
		params[ATK_PARAM].setValue(p.atk);
		params[DCY_PARAM].setValue(p.dcy);
		params[FMOD1_PARAM].setValue(p.fmod1);
		params[ATKF1_PARAM].setValue(p.atkf1);
		params[DCYF1_PARAM].setValue(p.dcyf1);
		params[FMOD2_PARAM].setValue(p.fmod2);
		params[ATKF2_PARAM].setValue(p.atkf2);
		params[DCYF2_PARAM].setValue(p.dcyf2);
		params[AMOD1_PARAM].setValue(p.amod1);
		params[ATKA1_PARAM].setValue(p.atka1);
		params[DCYA1_PARAM].setValue(p.dcya1);
		params[AMOD2_PARAM].setValue(p.amod2);
		params[ATKA2_PARAM].setValue(p.atka2);
		params[DCYA2_PARAM].setValue(p.dcya2);
		lastPreset = index;
	}

	void triggerChirp() {
		chirpLightPulse.trigger(0.1f);

		int numVoices = (int)std::round(params[VOICES_PARAM].getValue());
		numVoices = math::clamp(numVoices, 1, MAX_VOICES);

		float spreadCv = inputs[SPREADCV_INPUT].getVoltage() / 5.0f;
		float timeSpreadParam = math::clamp(params[TIME_SPREAD_PARAM].getValue() + spreadCv, 0.0f, 1.0f);
		float pitchJitterParam = params[PITCH_JITTER_PARAM].getValue();
		float panSpreadParam = params[PAN_SPREAD_PARAM].getValue();
		float envSpreadParam = params[ENV_SPREAD_PARAM].getValue();

		float fmCv = inputs[FMCV_INPUT].getVoltage() / 5.0f;
		float amCv = inputs[AMCV_INPUT].getVoltage() / 5.0f;
		float voct = inputs[VOCT_INPUT].getVoltage();

		BirdParams baseParams;
		baseParams.ifrq = math::clamp(params[IFRQ_PARAM].getValue(), 0.0f, 1.0f);
		baseParams.atk = math::clamp(params[ATK_PARAM].getValue(), 0.001f, 1.0f);
		baseParams.dcy = math::clamp(params[DCY_PARAM].getValue(), 0.001f, 1.0f);
		baseParams.fmod1 = math::clamp(params[FMOD1_PARAM].getValue(), 0.0f, 1.0f);
		baseParams.atkf1 = math::clamp(params[ATKF1_PARAM].getValue(), 0.001f, 1.0f);
		baseParams.dcyf1 = math::clamp(params[DCYF1_PARAM].getValue(), 0.001f, 1.0f);
		baseParams.fmod2 = math::clamp(params[FMOD2_PARAM].getValue(), 0.0f, 1.0f);
		baseParams.atkf2 = math::clamp(params[ATKF2_PARAM].getValue(), 0.001f, 1.0f);
		baseParams.dcyf2 = math::clamp(params[DCYF2_PARAM].getValue(), 0.001f, 1.0f);
		baseParams.amod1 = math::clamp(params[AMOD1_PARAM].getValue() + fmCv, 0.0f, 1.0f);
		baseParams.atka1 = math::clamp(params[ATKA1_PARAM].getValue(), 0.001f, 1.0f);
		baseParams.dcya1 = math::clamp(params[DCYA1_PARAM].getValue(), 0.001f, 1.0f);
		baseParams.amod2 = math::clamp(params[AMOD2_PARAM].getValue() + amCv, 0.0f, 1.0f);
		baseParams.atka2 = math::clamp(params[ATKA2_PARAM].getValue(), 0.001f, 1.0f);
		baseParams.dcya2 = math::clamp(params[DCYA2_PARAM].getValue(), 0.001f, 1.0f);

		for (int i = 0; i < numVoices; ++i) {
			BirdParams vp = baseParams;
			float delaySec = 0.0f;
			float panPos = 0.0f;

			if (i == 0) {
				// Lead bird
				delaySec = 0.0f;
				panPos = (numVoices > 1) ? -0.2f * panSpreadParam : 0.0f;
			} else {
				// Subsequent flock members
				float randPitch = (random::uniform() * 2.0f - 1.0f) * 0.15f * pitchJitterParam;
				vp.ifrq = math::clamp(vp.ifrq + randPitch, 0.0f, 1.0f);

				float randAtk = (random::uniform() * 2.0f - 1.0f) * 0.3f * envSpreadParam;
				float randDcy = (random::uniform() * 2.0f - 1.0f) * 0.3f * envSpreadParam;
				vp.atk = math::clamp(vp.atk + randAtk, 0.001f, 1.0f);
				vp.dcy = math::clamp(vp.dcy + randDcy, 0.001f, 1.0f);

				// Time delay spread (up to 1.5 seconds)
				delaySec = random::uniform() * 1.5f * timeSpreadParam;

				// Random stereo placement
				panPos = (random::uniform() * 2.0f - 1.0f) * panSpreadParam;
			}

			voices[i].trigger(vp, delaySec, voct, panPos);
		}
	}

	void process(const ProcessArgs& args) override {
		// Check preset change
		int currentPreset = (int)std::round(params[PRESET_PARAM].getValue());
		currentPreset = math::clamp(currentPreset, 0, 7);
		if (currentPreset != lastPreset) {
			applyPreset(currentPreset);
		}

		// Check if current parameters deviate from selected preset
		const BirdParams& ref = getBirdPreset(currentPreset);
		bool isModified = (
			std::abs(params[IFRQ_PARAM].getValue() - ref.ifrq) > 0.002f ||
			std::abs(params[ATK_PARAM].getValue() - ref.atk) > 0.002f ||
			std::abs(params[DCY_PARAM].getValue() - ref.dcy) > 0.002f ||
			std::abs(params[FMOD1_PARAM].getValue() - ref.fmod1) > 0.002f ||
			std::abs(params[ATKF1_PARAM].getValue() - ref.atkf1) > 0.002f ||
			std::abs(params[DCYF1_PARAM].getValue() - ref.dcyf1) > 0.002f ||
			std::abs(params[FMOD2_PARAM].getValue() - ref.fmod2) > 0.002f ||
			std::abs(params[ATKF2_PARAM].getValue() - ref.atkf2) > 0.002f ||
			std::abs(params[DCYF2_PARAM].getValue() - ref.dcyf2) > 0.002f ||
			std::abs(params[AMOD1_PARAM].getValue() - ref.amod1) > 0.002f ||
			std::abs(params[ATKA1_PARAM].getValue() - ref.atka1) > 0.002f ||
			std::abs(params[DCYA1_PARAM].getValue() - ref.dcya1) > 0.002f ||
			std::abs(params[AMOD2_PARAM].getValue() - ref.amod2) > 0.002f ||
			std::abs(params[ATKA2_PARAM].getValue() - ref.atka2) > 0.002f ||
			std::abs(params[DCYA2_PARAM].getValue() - ref.dcya2) > 0.002f
		);
		lights[MODIFIED_LIGHT].setBrightness(isModified ? 1.0f : 0.0f);

		// Trigger detection
		bool manualChirp = buttonTrigger.process(params[CHIRP_PARAM].getValue());
		bool externalChirp = trigTrigger.process(inputs[TRIG_INPUT].getVoltage());

		// Repeat trigger logic
		bool repeatEnabled = params[REPEAT_PARAM].getValue() > 0.5f;
		lights[REPEAT_LIGHT].setBrightness(repeatEnabled ? 1.0f : 0.0f);

		if (repeatEnabled) {
			repeatTimer += args.sampleTime;
			if (repeatTimer >= nextRepeatInterval) {
				repeatTimer = 0.0f;
				triggerChirp();

				float rateCv = inputs[RATECV_INPUT].getVoltage();
				float rateHz = params[RATE_PARAM].getValue() * std::pow(2.0f, rateCv);
				rateHz = math::clamp(rateHz, 0.05f, 50.0f);

				float baseInterval = 1.0f / rateHz;
				float jitterAmount = params[TIME_JITTER_PARAM].getValue();
				float jitter = (random::uniform() * 2.0f - 1.0f) * 0.5f * jitterAmount * baseInterval;
				nextRepeatInterval = std::max(0.02f, baseInterval + jitter);
			}
		} else {
			repeatTimer = 0.0f;
		}

		if (manualChirp || externalChirp) {
			triggerChirp();
		}

		// Process audio voices
		float mixL = 0.0f;
		float mixR = 0.0f;
		float maxEnv = 0.0f;

		int numVoices = (int)std::round(params[VOICES_PARAM].getValue());
		numVoices = math::clamp(numVoices, 1, MAX_VOICES);

		for (int i = 0; i < MAX_VOICES; ++i) {
			if (i < numVoices || voices[i].isPlaying()) {
				float vl = 0.0f, vr = 0.0f;
				voices[i].process(args.sampleTime, vl, vr);
				mixL += vl;
				mixR += vr;
				maxEnv = std::max(maxEnv, voices[i].getEnvelope());
			}
		}

		// Soft clipping / limiting to prevent extreme distortion when many voices overlap
		float gainScale = (numVoices > 1) ? (1.0f / std::sqrt((float)numVoices)) : 1.0f;
		mixL *= gainScale;
		mixR *= gainScale;

		outputs[OUTL_OUTPUT].setVoltage(mixL);
		outputs[OUTR_OUTPUT].setVoltage(mixR);
		outputs[ENV_OUTPUT].setVoltage(maxEnv * 10.0f);

		// Lights
		bool lightPulse = chirpLightPulse.process(args.sampleTime);
		lights[CHIRP_LIGHT].setBrightness(lightPulse ? 1.0f : (maxEnv * 0.5f));
	}
};

// Custom Widgets for Birds aesthetic
struct BirdScrew : app::SvgScrew {
	BirdScrew() {
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/BirdScrew.svg")));
	}
};

struct BirdPortIn : app::SvgPort {
	BirdPortIn() {
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/BirdPortIn.svg")));
	}
};

struct BirdPortOut : app::SvgPort {
	BirdPortOut() {
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/BirdPortOut.svg")));
	}
};

struct BirdKnobSmall : RoundKnob {
	BirdKnobSmall() {
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/BirdKnobSmall.svg")));
	}
};

struct BirdKnobMedium : RoundKnob {
	BirdKnobMedium() {
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/BirdKnobMedium.svg")));
	}
};

struct BirdKnobSnap : RoundKnob {
	BirdKnobSnap() {
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/BirdKnobSnap.svg")));
		snap = true;
	}
};

struct BirdButton : app::SvgSwitch {
	BirdButton() {
		momentary = true;
		addFrame(Svg::load(asset::plugin(pluginInstance, "res/BirdButton_0.svg")));
		addFrame(Svg::load(asset::plugin(pluginInstance, "res/BirdButton_1.svg")));
	}
};

struct BirdSwitch : app::SvgSwitch {
	BirdSwitch() {
		addFrame(Svg::load(asset::plugin(pluginInstance, "res/BirdSwitch_0.svg")));
		addFrame(Svg::load(asset::plugin(pluginInstance, "res/BirdSwitch_1.svg")));
	}
};

struct BirdsWidget : ModuleWidget {
	BirdsWidget(Birds* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Birds.svg")));

		// Custom Screws
		addChild(createWidget<BirdScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<BirdScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<BirdScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<BirdScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// Header Controls
		addChild(createLightCentered<SmallLight<YellowLight>>(Vec(176, 38), module, Birds::MODIFIED_LIGHT));
		addParam(createParamCentered<BirdKnobSnap>(Vec(195, 38), module, Birds::PRESET_PARAM));
		addParam(createParamCentered<BirdButton>(Vec(245, 38), module, Birds::CHIRP_PARAM));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(268, 17), module, Birds::CHIRP_LIGHT));
		addParam(createParamCentered<BirdSwitch>(Vec(305, 38), module, Birds::REPEAT_PARAM));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(320, 38), module, Birds::REPEAT_LIGHT));
		addParam(createParamCentered<BirdKnobMedium>(Vec(365, 38), module, Birds::RATE_PARAM));

		// Synthesis Matrix Knobs (5 evenly spaced rows of 48px at y = 88, 136, 184, 232, 280)
		const float colX[3] = {54.f, 122.f, 190.f};
		const float rowY[5] = {88.f, 136.f, 184.f, 232.f, 280.f};

		// Row 1: Carrier & Amp
		addParam(createParamCentered<BirdKnobSmall>(Vec(colX[0], rowY[0]), module, Birds::IFRQ_PARAM));
		addParam(createParamCentered<BirdKnobSmall>(Vec(colX[1], rowY[0]), module, Birds::ATK_PARAM));
		addParam(createParamCentered<BirdKnobSmall>(Vec(colX[2], rowY[0]), module, Birds::DCY_PARAM));

		// Row 2: FM Frequency
		addParam(createParamCentered<BirdKnobSmall>(Vec(colX[0], rowY[1]), module, Birds::FMOD1_PARAM));
		addParam(createParamCentered<BirdKnobSmall>(Vec(colX[1], rowY[1]), module, Birds::ATKF1_PARAM));
		addParam(createParamCentered<BirdKnobSmall>(Vec(colX[2], rowY[1]), module, Birds::DCYF1_PARAM));

		// Row 3: AM Frequency
		addParam(createParamCentered<BirdKnobSmall>(Vec(colX[0], rowY[2]), module, Birds::FMOD2_PARAM));
		addParam(createParamCentered<BirdKnobSmall>(Vec(colX[1], rowY[2]), module, Birds::ATKF2_PARAM));
		addParam(createParamCentered<BirdKnobSmall>(Vec(colX[2], rowY[2]), module, Birds::DCYF2_PARAM));

		// Row 4: FM Gain
		addParam(createParamCentered<BirdKnobSmall>(Vec(colX[0], rowY[3]), module, Birds::AMOD1_PARAM));
		addParam(createParamCentered<BirdKnobSmall>(Vec(colX[1], rowY[3]), module, Birds::ATKA1_PARAM));
		addParam(createParamCentered<BirdKnobSmall>(Vec(colX[2], rowY[3]), module, Birds::DCYA1_PARAM));

		// Row 5: AM Gain
		addParam(createParamCentered<BirdKnobSmall>(Vec(colX[0], rowY[4]), module, Birds::AMOD2_PARAM));
		addParam(createParamCentered<BirdKnobSmall>(Vec(colX[1], rowY[4]), module, Birds::ATKA2_PARAM));
		addParam(createParamCentered<BirdKnobSmall>(Vec(colX[2], rowY[4]), module, Birds::DCYA2_PARAM));

		// Flock Controls (Right Side)
		addParam(createParamCentered<BirdKnobSnap>(Vec(268, 100), module, Birds::VOICES_PARAM));
		addParam(createParamCentered<BirdKnobMedium>(Vec(356, 100), module, Birds::PITCH_JITTER_PARAM));

		addParam(createParamCentered<BirdKnobMedium>(Vec(268, 170), module, Birds::TIME_SPREAD_PARAM));
		addParam(createParamCentered<BirdKnobMedium>(Vec(356, 170), module, Birds::PAN_SPREAD_PARAM));

		addParam(createParamCentered<BirdKnobMedium>(Vec(268, 240), module, Birds::TIME_JITTER_PARAM));
		addParam(createParamCentered<BirdKnobMedium>(Vec(356, 240), module, Birds::ENV_SPREAD_PARAM));

		// Bottom Jack Bay (Shifted up to jackY = 336.f to clear bottom corner screws)
		const float jackY = 336.f;
		addInput(createInputCentered<BirdPortIn>(Vec(34, jackY), module, Birds::TRIG_INPUT));
		addInput(createInputCentered<BirdPortIn>(Vec(72, jackY), module, Birds::VOCT_INPUT));
		addInput(createInputCentered<BirdPortIn>(Vec(110, jackY), module, Birds::FMCV_INPUT));
		addInput(createInputCentered<BirdPortIn>(Vec(148, jackY), module, Birds::AMCV_INPUT));
		addInput(createInputCentered<BirdPortIn>(Vec(186, jackY), module, Birds::RATECV_INPUT));
		addInput(createInputCentered<BirdPortIn>(Vec(224, jackY), module, Birds::SPREADCV_INPUT));

		addOutput(createOutputCentered<BirdPortOut>(Vec(282, jackY), module, Birds::ENV_OUTPUT));
		addOutput(createOutputCentered<BirdPortOut>(Vec(328, jackY), module, Birds::OUTL_OUTPUT));
		addOutput(createOutputCentered<BirdPortOut>(Vec(372, jackY), module, Birds::OUTR_OUTPUT));
	}

	void appendContextMenu(Menu* menu) override {
		Birds* module = dynamic_cast<Birds*>(this->module);
		if (!module) return;

		menu->addChild(new MenuSeparator);
		menu->addChild(createMenuLabel("Bird Species Presets"));

		for (int i = 0; i < 8; ++i) {
			menu->addChild(createMenuItem(getBirdPresetName(i), "", [=]() {
				module->params[Birds::PRESET_PARAM].setValue(i);
				module->applyPreset(i);
			}));
		}
	}
};

Model* modelBirds = createModel<Birds, BirdsWidget>("Birds");
