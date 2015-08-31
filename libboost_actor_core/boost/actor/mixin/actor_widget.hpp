/******************************************************************************\
 *                                                                            *
 *           ____                  _        _        _                        *
 *          | __ )  ___   ___  ___| |_     / \   ___| |_ ___  _ __            *
 *          |  _ \ / _ \ / _ \/ __| __|   / _ \ / __| __/ _ \| '__|           *
 *          | |_) | (_) | (_) \__ \ |_ _ / ___ \ (__| || (_) | |              *
 *          |____/ \___/ \___/|___/\__(_)_/   \_\___|\__\___/|_|              *
 *                                                                            *
 *                                                                            *
 *                                                                            *
 * Copyright (C) 2011 - 2015                                                  *
 * Dominik Charousset <dominik.charousset (at) haw-hamburg.de>                *
 *                                                                            *
 * Distributed under the terms and conditions of the BSD 3-Clause License or  *
 * (at your option) under the terms and conditions of the Boost Software      *
 * License 1.0. See accompanying files LICENSE and LICENSE_ALTERNATIVE.       *
 *                                                                            *
 * If you did not receive a copy of the license files, see                    *
 * http://opensource.org/licenses/BSD-3-Clause and                            *
 * http://www.boost.org/LICENSE_1_0.txt.                                      *
 ******************************************************************************/

#ifndef BOOST_ACTOR_MIXIN_ACTOR_WIDGET_HPP
#define BOOST_ACTOR_MIXIN_ACTOR_WIDGET_HPP

#include "boost/actor/config.hpp"
#include "boost/actor/make_counted.hpp"
#include "boost/actor/actor_companion.hpp"
#include "boost/actor/message_handler.hpp"

BOOST_ACTOR_PUSH_WARNINGS
#include <QEvent>
#include <QApplication>
BOOST_ACTOR_POP_WARNINGS

namespace boost {
namespace actor {
namespace mixin {

template<typename Base, int EventId = static_cast<int>(QEvent::User + 31337)>
class actor_widget : public Base {
public:
  typedef typename actor_companion::message_pointer message_pointer;

  struct event_type : public QEvent {
    message_pointer mptr;
    event_type(message_pointer ptr)
        : QEvent(static_cast<QEvent::Type>(EventId)), mptr(std::move(ptr)) {
      // nop
    }
  };

  template <typename... Ts>
  actor_widget(Ts&&... xs) : Base(std::forward<Ts>(xs)...) {
    companion_ = make_counted<actor_companion>();
    companion_->on_enqueue([=](message_pointer ptr) {
      qApp->postEvent(this, new event_type(std::move(ptr)));
    });
  }

  template <typename T>
  void set_message_handler(T pfun) {
    companion_->become(pfun(companion_.get()));
  }

  bool event(QEvent* event) override {
    if (event->type() == static_cast<QEvent::Type>(EventId)) {
      auto ptr = dynamic_cast<event_type*>(event);
      if (ptr) {
        companion_->invoke_message(ptr->mptr,
                                    companion_->get_behavior(),
                                    companion_->awaited_response_id());
        return true;
      }
    }
    return Base::event(event);
  }

  actor as_actor() const {
    return companion_;
  }

private:
    actor_companion_ptr companion_;
};

} // namespace mixin
} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_MIXIN_ACTOR_WIDGET_HPP
