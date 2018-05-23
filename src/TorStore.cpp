/******************************************************
**
** Reference example for storing and sending using
** the RawInputPort and RawOutputPort objects.
**
** In this demo we have 
**
** 	completed method
**	received method
**	error method
**	send method
**	isBusy method
**
*******************************************************/

#include "TorpedoDemo.hpp"
#include <mutex>
#include "dsp/digital.hpp"
				// torpedo.hpp is the only include necessary 
				// to use torpedo. Your project should also
				// compile and link torpedo.cpp
#include "torpedo.hpp"

struct TorStore;		// Forward declarations, because I want to
				// include a reference to the module in 
				// the ports.

	//
	// I have to subclass the StoreInputPort so that I can override the
	// received method to actually get at received messages
	//
struct TorStoreInputPort : Torpedo::RawInputPort {
	TorStore *tpModule;
	TorStoreInputPort(TorStore *module, unsigned int portNum):Torpedo::RawInputPort((Module *)module, portNum) {tpModule = module;};
	void received(std::string appId, std::string message) override;
};

struct TorStore : Module  {
	static const int deviceCount = 10;
	enum ParamIds {
		PARAM_SEND_1,		// Push button to send message
		PARAM_SEND_2,
		PARAM_SEND_3,
		PARAM_SEND_4,
		PARAM_SEND_5,
		PARAM_SEND_6,
		PARAM_SEND_7,
		PARAM_SEND_8,
		PARAM_SEND_9,
		PARAM_SEND_10,
		NUM_PARAMS
	};
	enum InputIds {
		INPUT_TOR_1,		// An input port for torpedo to use
		INPUT_TOR_2,
		INPUT_TOR_3,
		INPUT_TOR_4,
		INPUT_TOR_5,
		INPUT_TOR_6,
		INPUT_TOR_7,
		INPUT_TOR_8,
		INPUT_TOR_9,
		INPUT_TOR_10,
		NUM_INPUTS
	};
	enum OutputIds {
		OUTPUT_TOR_1,		// An output port for torpedo to use
		OUTPUT_TOR_2,
		OUTPUT_TOR_3,
		OUTPUT_TOR_4,
		OUTPUT_TOR_5,
		OUTPUT_TOR_6,
		OUTPUT_TOR_7,
		OUTPUT_TOR_8,
		OUTPUT_TOR_9,
		OUTPUT_TOR_10,
		NUM_OUTPUTS
	};
	enum LightIds {
		LIGHT_STORE_1,		// Store light 
		LIGHT_STORE_2, 
		LIGHT_STORE_3, 
		LIGHT_STORE_4, 
		LIGHT_STORE_5, 
		LIGHT_STORE_6, 
		LIGHT_STORE_7, 
		LIGHT_STORE_8, 
		LIGHT_STORE_9, 
		LIGHT_STORE_10, 
		NUM_LIGHTS
	};

	std::string apps[deviceCount] = {"","","","","","","","","",""};
	std::string messages[deviceCount] = {"","","","","","","","","",""};
	TorStoreInputPort inPorts[deviceCount] = {
		TorStoreInputPort(this, INPUT_TOR_1),
		TorStoreInputPort(this, INPUT_TOR_2),
		TorStoreInputPort(this, INPUT_TOR_3),
		TorStoreInputPort(this, INPUT_TOR_4),
		TorStoreInputPort(this, INPUT_TOR_5),
		TorStoreInputPort(this, INPUT_TOR_6),
		TorStoreInputPort(this, INPUT_TOR_7),
		TorStoreInputPort(this, INPUT_TOR_8),
		TorStoreInputPort(this, INPUT_TOR_9),
		TorStoreInputPort(this, INPUT_TOR_10)
	};
	Torpedo::RawOutputPort outPorts[deviceCount] = {
		Torpedo::RawOutputPort(this, OUTPUT_TOR_1),
		Torpedo::RawOutputPort(this, OUTPUT_TOR_2),
		Torpedo::RawOutputPort(this, OUTPUT_TOR_3),
		Torpedo::RawOutputPort(this, OUTPUT_TOR_4),
		Torpedo::RawOutputPort(this, OUTPUT_TOR_5),
		Torpedo::RawOutputPort(this, OUTPUT_TOR_6),
		Torpedo::RawOutputPort(this, OUTPUT_TOR_7),
		Torpedo::RawOutputPort(this, OUTPUT_TOR_8),
		Torpedo::RawOutputPort(this, OUTPUT_TOR_9),
		Torpedo::RawOutputPort(this, OUTPUT_TOR_10),
	};
	SchmittTrigger triggers[deviceCount];

	TorStore() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}

	void step() override;
	json_t *toJson() override;
	void fromJson(json_t *rootJ) override;
};

void TorStore::step() {
	for (int i = 0; i < deviceCount; i++) {
		if (triggers[i].process(params[TorStore::PARAM_SEND_1 + i].value)) {
			if (apps[i].length() > 0) {
				outPorts[i].send(apps[i], messages[i]);
			}
		}
		lights[LIGHT_STORE_1 + i].value = (apps[i].length() > 0);
		outPorts[i].process();
		inPorts[i].process();
	}
}

json_t *TorStore::toJson(void) {
	json_t *rootJ = json_object();
	json_t *array = json_array();
	for (int i = 0; i < deviceCount; i++) {
		json_array_append_new(array, json_string(apps[i].c_str()));
		json_array_append_new(array, json_string(messages[i].c_str()));
	}
	json_object_set_new(rootJ, "messages", array);
	return rootJ;
}

void TorStore::fromJson(json_t *rootJ) {
	json_t *j0 = json_object_get(rootJ, "messages");
	if (j0) {
		int size = json_array_size(j0);
		if (size > (deviceCount * 2))
			size = (deviceCount * 2);
		if (size % 2)
			size -= 1;
		for (int i = 0; i < deviceCount; i++) {
			json_t *j1 = json_array_get(j0, i * 2);
			if (j1)
				apps[i].assign(json_string_value(j1));
			json_t *j2 = json_array_get(j0, i * 2 + 1);
			if (j2)
				messages[i].assign(json_string_value(j2));
		}
	}
}

	//
	// This received method is called whenever the Raw receives
	// a message.
	//
void TorStoreInputPort::received(std::string appId, std::string message) {
	int portNum = _portNum - TorStore::INPUT_TOR_1;
	tpModule->apps[portNum].assign(appId);
	tpModule->messages[portNum].assign(message);
}

struct TorStoreWidget : ModuleWidget {

	TorStoreWidget(TorStore *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin, "res/TorStore.svg")));
		for (int i = 0; i < TorStore::deviceCount; i++) {
			addInput(Port::create<sub_port_black>(Vec(4,30 + 30 * i), Port::INPUT, module, TorStore::INPUT_TOR_1 + i));

			addOutput(Port::create<sub_port_black>(Vec(92,30 + 30 * i), Port::OUTPUT, module, TorStore::OUTPUT_TOR_1 + i));

			addParam(ParamWidget::create<sub_btn_moment>(Vec(67, 35 + 30 * i), module, TorStore::PARAM_SEND_1 + i, 0.0f, 1.0f, 0.0f));

			addChild(ModuleLightWidget::create<LargeLight<GreenLight>>(Vec(37, 35 + 30 * i), module, TorStore::LIGHT_STORE_1 + i));
		}
	}
};

Model *modelTorStore = Model::create<TorStore, TorStoreWidget>("TorpedoDemo", "Torpedo Store Demo", "Torpedo Store Demo", UTILITY_TAG);
