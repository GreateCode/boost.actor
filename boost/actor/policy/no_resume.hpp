#ifndef NO_RESUME_HPP
#define NO_RESUME_HPP

#include <chrono>
#include <utility>

#include "boost/actor/exception.hpp"
#include "boost/actor/exit_reason.hpp"
#include "boost/actor/policy/resume_policy.hpp"

namespace boost {
namespace actor { namespace policy {

// this policy simply forwards calls to @p await_data to the scheduling
// policy and throws an exception whenever @p resume is called;
// it intentionally works only with the no_scheduling policy
class no_resume {

 public:

    template<class Base, class Derived>
    struct mixin : Base {

        template<typename... Ts>
        mixin(Ts&&... args)
                : Base(std::forward<Ts>(args)...)
                , m_hidden(true) { }

        inline void attach_to_scheduler() {
            this->ref();
        }

        inline void detach_from_scheduler() {
            this->deref();
        }

        inline resumable::resume_result resume(execution_unit*) {
            auto done_cb = [=](uint32_t reason) {
                this->planned_exit_reason(reason);
                this->on_exit();
                this->cleanup(reason);
            };
            try {
                this->act();
                done_cb(exit_reason::normal);
            }
            catch (actor_exited& e) {
                done_cb(e.reason());
            }
            catch (...) {
                done_cb(exit_reason::unhandled_exception);
            }
            return resumable::done;
        }

        bool m_hidden;

    };

    template<class Actor>
    void await_ready(Actor* self) {
        self->await_data();
    }

};

} } // namespace actor
} // namespace boost::policy

#endif // NO_RESUME_HPP
