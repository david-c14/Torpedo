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
		OUTPUT_V,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	TorPatch() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	void step() override;
};

void TorPatch::step() {
}

struct TorPatchWidget : ModuleWidget {
	TorPatchWidget(TorPatch *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin, "res/LD-106.svg")));

		addInput(Port::create<sub_port>(Vec(4,77), Port::INPUT, module, TorPatch::INPUT_TOR));

		addOutput(Port::create<sub_port_blue>(Vec(62,77), Port::OUTPUT, module, TorPatch::OUTPUT_TOR));

		addParam(ParamWidget::create<sub_knob_small>(Vec(4, 105), module, TorPatch::PARAM_1, -10.0f, 10.0f, 5.0f));
	}
};

Model *modelTorPatch = Model::create<TorPatch, TorPatchWidget>("TorpedoDemo", "Torpedo Patch Demo", "Torpedo Patch Demo", LOGIC_TAG);
