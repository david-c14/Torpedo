#pragma once
#include "rack.hpp"
#include "deque"
using namespace rack;

namespace Torpedo {
	// 
	// Basic shared functionality
	//

	struct BasePort {
		enum States {
			STATE_QUIESCENT,
			STATE_HEADER,
			STATE_BODY,
			STATE_TRAILER,
			STATE_ABORTING
		};
	
		enum Errors {
			ERROR_STATE,
			ERROR_COUNTER,
			ERROR_LENGTH,
			ERROR_CHECKSUM
		};
	
		unsigned int _state = STATE_QUIESCENT;
		unsigned int _checksum = 0;
		unsigned int dbg = 0;
		Module *_module;
		unsigned int _portNum;
		BasePort(Module *module, unsigned int portNum) {
			_module = module;
			_portNum = portNum;	
		}
		void raiseError(unsigned int errorType);
		virtual void error(unsigned int errorType) {};
		virtual int isBusy(void) {
			return (_state != STATE_QUIESCENT);
		}
		void addCheckSum(unsigned int byte, unsigned int counter);
		
	};
	
	//
	// Raw output port functionality. Encapsulating layers 2-5 of the OSI model
	//

	struct RawOutputPort : BasePort {
		Output *_port;
		virtual void abort();
		virtual void send(std::string appId, std::string message);
		virtual void send(std::string message);
		virtual void process();
		virtual void completed();
		RawOutputPort(Module *module, unsigned int portNum) : BasePort(module, portNum) {
			_port = &(_module->outputs[_portNum]);
		}
		std::string _message;
		std::string _appId;
		unsigned int _counter;
		virtual void appId(std::string app) { _appId.assign(app); }
	};

	//
	// Raw input port functionality. Encapsulating layers 2-5 of the OSI model
	//
	
	struct RawInputPort : BasePort {
		Input *_port;
		void process();
		virtual void received(std::string appId, std::string message);
		RawInputPort(Module *module, unsigned int portNum) : BasePort(module, portNum) { 
			_port = &(_module->inputs[_portNum]);
		}
		std::string _message;
		std::string _appId;
		unsigned int _counter;
		unsigned int _length;
	};

	//
	// Basic text sending.
	//

	struct TextInputPort : RawInputPort {
		TextInputPort(Module *module, unsigned int portNum) : RawInputPort(module, portNum) {}
		void received(std::string appId, std::string message) override;
		virtual void received(std::string message) {}
	};

	struct TextOutputPort : RawOutputPort {
		TextOutputPort(Module *module, unsigned int portNum) : RawOutputPort(module, portNum) {_appId.assign("TEXT");}
	};

	//
	// Queued sending.
	//

	struct QueuedOutputPort : RawOutputPort {
		QueuedOutputPort(Module *module, unsigned int portNum) : RawOutputPort(module, portNum) {}
		std::deque<std::string *> _queue;
		unsigned int _replace = 0;
		void replace(unsigned int rep) { _replace = rep; }
		unsigned int _size = 0;
		void size(unsigned int s);
		virtual ~QueuedOutputPort() { for (auto i : _queue) delete i; }
		void send(std::string message) override;
		void process() override;
		int isBusy() override { return (_state != STATE_QUIESCENT) || _queue.size(); }
		virtual int isFul() { return _queue.size() >= _size; }
		void abort() override;
	};

	//
	// Addressed Messages.
	//

	struct MessageOutputPort : QueuedOutputPort {
		MessageOutputPort(Module *module, unsigned int portNum) : QueuedOutputPort(module, portNum) {_appId.assign("MESG");}
		virtual void send(std::string pluginName, std::string moduleName, std::string message);
	};

	struct MessageInputPort : RawInputPort {
		MessageInputPort(Module *module, unsigned int portNum) : RawInputPort(module, portNum) {}
		void received(std::string appId, std::string message) override;
		virtual void received(std::string pluginName, std::string moduleName, std::string message) {}
	};

	//
	// Device Patches.
	//

	struct PatchOutputPort : QueuedOutputPort {
		PatchOutputPort(Module *module, unsigned int portNum) : QueuedOutputPort(module, portNum) {_appId.assign("PTCH");}
		virtual void send(std::string pluginName, std::string moduleName, json_t *rootJ);
	};

	struct PatchInputPort : RawInputPort {
		PatchInputPort(Module *module, unsigned int portNum) : RawInputPort(module, portNum) {}
		void received(std::string appId, std::string message) override;
		virtual void received(std::string pluginName, std::string moduleName, json_t *rootJ) {}
	};
		
}
	
