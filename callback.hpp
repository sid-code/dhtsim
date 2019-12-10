#ifndef DHTSIM_CALLBACK_H
#define DHTSIM_CALLBACK_H

#include <functional>
namespace dhtsim {

template <typename SuccessParameter,
	  typename FailureParameter = SuccessParameter>
class CallbackSet {
public:
	using SuccessFn = std::function<void(SuccessParameter)>;
	using FailureFn = std::function<void(FailureParameter)>;
	// Main constructors
        CallbackSet() : successFns({}), failureFns({}) {}
        CallbackSet(const std::vector<SuccessFn>& successFns,
			const std::vector<FailureFn>& failureFns)
		: successFns(successFns),
		  failureFns(failureFns) {};
	CallbackSet(const SuccessFn& successFn,
		    const FailureFn& failureFn) {
		this->successFns.push_back(successFn);
		this->failureFns.push_back(failureFn);
	}

	// Static constructors
	static CallbackSet onSuccess(SuccessFn fn) {
		CallbackSet s;
		s.successFns.push_back(fn);
		return s;
	}
	static CallbackSet onFailure(FailureFn fn) {
		CallbackSet s;
		s.failureFns.push_back(fn);
		return s;
	}

	void success(SuccessParameter m) const {
		for (const auto& sf : this->successFns) {
			sf(m);
		}
	}
	void failure(FailureParameter m) const {
		for (const auto& ff : this->failureFns) {
			ff(m);
		}
	}

	bool empty() const {
		return this->successFns.empty() && this->failureFns.empty();
	}

	void operator+=(const CallbackSet& other) {
		this->successFns.insert(this->successFns.end(),
					other.successFns.begin(), other.successFns.end());
		this->failureFns.insert(this->failureFns.end(),
					other.failureFns.begin(), other.failureFns.end());
	}
	friend CallbackSet operator+(const CallbackSet& c1, const CallbackSet& c2) {
		CallbackSet result;
		result += c1;
		result += c2;
		return result;
	}
private:
	std::vector<SuccessFn> successFns;
	std::vector<FailureFn> failureFns;
};
}

#endif
