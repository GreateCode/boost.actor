#include <exception>

#include "boost/actor/all.hpp"
#include "boost/actor/mixin/actor_widget.hpp"

BOOST_ACTOR_PUSH_WARNINGS
#include <QWidget>
#include <QLineEdit>
#include <QTextEdit>
BOOST_ACTOR_POP_WARNINGS

class ChatWidget : public boost::actor::mixin::actor_widget<QWidget> {

    Q_OBJECT

    typedef boost::actor::mixin::actor_widget<QWidget> super;

public:

    ChatWidget(QWidget* parent = nullptr, Qt::WindowFlags f = 0);

 public slots:

    void sendChatMessage();
    void joinGroup();
    void changeName();

private:

    template<typename T>
    T* get(T*& member, const char* name) {
        if (member == nullptr) {
            member = findChild<T*>(name);
            if (member == nullptr)
                throw std::runtime_error("unable to find child: "
                                         + std::string(name));
        }
        return member;
    }

    inline QLineEdit* input() {
        return get(input_, "input");
    }

    inline QTextEdit* output() {
        return get(output_, "output");
    }

    inline void print(const QString& what) {
        output()->append(what);
    }

    QLineEdit* input_;
    QTextEdit* output_;
    std::string name_;
    boost::actor::group chatroom_;

};
