#include <EventManager.h>

namespace AMC {

	EventManager::EventManager(const std::vector<std::tuple<std::string, float, float, UpdateCallback, EasingFunction>>& events) : currentTime(0.0f){
		for (auto& event : events) {
			const std::string& eventId = std::get<0>(event);
			float startTime = std::get<1>(event);
			float duration = std::get<2>(event);
			UpdateCallback updatefunc = std::get<3>(event);
			EasingFunction easingfunc = std::get<4>(event);

			// Hack because for some reason std::get considers nullptr as valid function pointer instead of empty
			//if (!easingfunc) {
			//	easingfunc = [](float t) {return t; }; // Linear Interpolation
			//}

			events_t *e = new events_t();
			e->start = startTime;
			e->duration = duration;
			e->deltaT = 0.0f;
			e->easingFunction = easingfunc;
			e->updateFunction = updatefunc;
			eventList.emplace(eventId, e);
		}
	}

	EventManager::~EventManager(){
		eventList.clear();
	}

	void EventManager::resetEvents(){
		currentTime = 0.0f;
		recalculateTs();
	}

	inline float EventManager::getEventTime(std::string eventName){
		return eventList[eventName]->deltaT;
	}

	inline float EventManager::getCurrentTime(){
		return currentTime;
	}

	void EventManager::update(){
		if (AMC::ANIMATING) {
			currentTime += (float)AMC::deltaTime;
			recalculateTs();
		}
	}

	void EventManager::recalculateTs(){
		for (std::pair<std::string, events_t*> ev : eventList) {
			events_t* e = ev.second;
			float t = (currentTime - e->start) / e->duration;
			// only pdate if event is active
			if (t >= 0.0f && t <= 1.0f) {
				e->deltaT = std::clamp(t, 0.0f, 1.0f);

				float adjustedT = e->deltaT;
				if (e->easingFunction) {
					adjustedT = e->easingFunction(e->deltaT);
				}

				if (e->updateFunction) {
					e->updateFunction(adjustedT);
				}
			}
		}
	}
	EventManager& operator+=(EventManager& e, float t){
		e.currentTime += t;
		e.recalculateTs();
		return e;
	}
	EventManager& operator-=(EventManager& e, float t){
		e.currentTime -= t;
		e.recalculateTs();
		return e;
	}

	float& EventManager::operator[](std::string eventName) {
		return eventList[eventName]->deltaT;
	}
};