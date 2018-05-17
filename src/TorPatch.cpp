#include "TorpedoDemo.hpp"
#include "torpedo.hpp"

struct TorPatchOutputPort : Torpedo::PatchOutputPort {
	TorPatchOutputPort(Module *module, unsigned int portNum):Torpedo::PatchOutputPort(module, portNum) {};
	void send(std::string pluginName, std::string moduleName, json_t *rootJ) override;
};

struct TorPatch : Module  {
	enum ParamIds {
		PARAM_1,
		PARAM_2,
		PARAM_3,
		NUM_PARAMS
	};
	enum InputIds {
		INPUT_TOR,
		NUM_INPUTS
	};
	enum OutputIds {
		OUTPUT_TOR,
		OUTPUT_V1,
		OUTPUT_V2,
		NUM_OUTPUTS
	};
	enum LightIds {
		LIGHT_GREEN,
		LIGHT_RED,
		NUM_LIGHTS
	};

	TorPatch() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	void step() override;
};

void TorPatch::step() {
	outputs[OUTPUT_V1].value = params[PARAM_1].value;
	outputs[OUTPUT_V2].value = params[PARAM_2].value;
	lights[LIGHT_RED].value = (params[PARAM_3].value > 0.5f);
	lights[LIGHT_GREEN].value = (params[PARAM_3].value < 1.5f);
}

struct TorPatchWidget : ModuleWidget {
	TorPatchWidget(TorPatch *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin, "res/TorPatch.svg")));

		addInput(Port::create<sub_port_black>(Vec(4,19), Port::INPUT, module, TorPatch::INPUT_TOR));

		addOutput(Port::create<sub_port_black>(Vec(92,19), Port::OUTPUT, module, TorPatch::OUTPUT_TOR));

		addOutput(Port::create<sub_port>(Vec(4,149), Port::OUTPUT, module, TorPatch::OUTPUT_V1));
		addOutput(Port::create<sub_port>(Vec(92,149), Port::OUTPUT, module, TorPatch::OUTPUT_V2));

		addParam(ParamWidget::create<sub_knob_med>(Vec(4, 105), module, TorPatch::PARAM_1, -5.0f, 5.0f, 0.0f));
		addParam(ParamWidget::create<sub_knob_med>(Vec(79, 105), module, TorPatch::PARAM_2, -5.0f, 5.0f, 0.0f));
		addParam(ParamWidget::create<sub_sw_3>(Vec(53, 105), module, TorPatch::PARAM_3, 0.0f, 2.0f, 0.0f));

		addChild(ModuleLightWidget::create<LargeLight<GreenRedLight>>(Vec(53, 140), module, TorPatch::LIGHT_GREEN));
	}
};

Model *modelTorPatch = Model::create<TorPatch, TorPatchWidget>("TorpedoDemo", "Torpedo Patch Demo", "Torpedo Patch Demo", LOGIC_TAG);
