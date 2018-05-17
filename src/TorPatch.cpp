#include "TorpedoDemo.hpp"
#include "torpedo.hpp"

struct TorPatch;

struct TorPatchOutputPort : Torpedo::PatchOutputPort {
	TorPatchOutputPort(Module *module, unsigned int portNum):Torpedo::PatchOutputPort(module, portNum) {};
};

struct TorPatchInputPort : Torpedo::PatchInputPort {
	TorPatch *tpModule;
	TorPatchInputPort(TorPatch *module, unsigned int portNum):Torpedo::PatchInputPort((Module *)module, portNum) {tpModule = module;};
	void received(std::string pluginName, std::string moduleName, json_t *rootJ) override;
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
	int isDirty = 0;
	TorPatchOutputPort outPort = TorPatchOutputPort(this, OUTPUT_TOR);
	TorPatchInputPort inPort = TorPatchInputPort(this, INPUT_TOR);
	TorPatch() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	void step() override;
};

void TorPatch::step() {
	outputs[OUTPUT_V1].value = params[PARAM_1].value;
	outputs[OUTPUT_V2].value = params[PARAM_2].value;
	lights[LIGHT_RED].value = (params[PARAM_3].value > 0.5f);
	lights[LIGHT_GREEN].value = (params[PARAM_3].value < 1.5f);
	if (!outPort.isBusy()) {
		json_t *rootJ = json_object();
		json_object_set_new(rootJ, "param1", json_real(params[PARAM_1].value));
		json_object_set_new(rootJ, "param2", json_real(params[PARAM_2].value));
		json_object_set_new(rootJ, "param3", json_real(params[PARAM_3].value));
		outPort.send(std::string(TOSTRING(SLUG)), std::string("TorPatch"), rootJ);
	}
	outPort.process();
	inPort.process();
}

void TorPatchInputPort::received(std::string pluginName, std::string moduleName, json_t *rootJ) {
	if (pluginName.compare(TOSTRING(SLUG))) return;
	if (moduleName.compare("TorPatch")) return;
	json_t *j1 = json_object_get(rootJ, "param1");
	if (j1)
		engineSetParam(_module, TorPatch::PARAM_1, json_number_value(j1));
	json_t *j2 = json_object_get(rootJ, "param2");
	if (j2)
		engineSetParam(_module, TorPatch::PARAM_2, json_number_value(j2));
	json_t *j3 = json_object_get(rootJ, "param3");
	if (j3)
		engineSetParam(_module, TorPatch::PARAM_3, json_number_value(j3));
	tpModule->isDirty = true;
}

struct TorPatchWidget : ModuleWidget {
	ParamWidget *p1;
	ParamWidget *p2;
	ParamWidget *p3;
	TorPatch *tpModule;
	TorPatchWidget(TorPatch *module) : ModuleWidget(module) {
		tpModule = module;
		setPanel(SVG::load(assetPlugin(plugin, "res/TorPatch.svg")));

		addInput(Port::create<sub_port_black>(Vec(4,19), Port::INPUT, module, TorPatch::INPUT_TOR));

		addOutput(Port::create<sub_port_black>(Vec(92,19), Port::OUTPUT, module, TorPatch::OUTPUT_TOR));

		addOutput(Port::create<sub_port>(Vec(4,149), Port::OUTPUT, module, TorPatch::OUTPUT_V1));
		addOutput(Port::create<sub_port>(Vec(92,149), Port::OUTPUT, module, TorPatch::OUTPUT_V2));

		p1 = ParamWidget::create<sub_knob_med>(Vec(4, 105), module, TorPatch::PARAM_1, -5.0f, 5.0f, 0.0f);
		addParam(p1);
		p2 = ParamWidget::create<sub_knob_med>(Vec(79, 105), module, TorPatch::PARAM_2, -5.0f, 5.0f, 0.0f);
		addParam(p2);
		p3 = ParamWidget::create<sub_sw_3>(Vec(53, 105), module, TorPatch::PARAM_3, 0.0f, 2.0f, 0.0f);
		addParam(p3);

		addChild(ModuleLightWidget::create<LargeLight<GreenRedLight>>(Vec(53, 140), module, TorPatch::LIGHT_GREEN));
	}
	void step() override {
		if (tpModule->isDirty) {
			p1->setValue(tpModule->params[TorPatch::PARAM_1].value);	
			p2->setValue(tpModule->params[TorPatch::PARAM_2].value);	
			p3->setValue(tpModule->params[TorPatch::PARAM_3].value);	
			tpModule->isDirty = 0;
		}
	}
};

Model *modelTorPatch = Model::create<TorPatch, TorPatchWidget>("TorpedoDemo", "Torpedo Patch Demo", "Torpedo Patch Demo", LOGIC_TAG);
