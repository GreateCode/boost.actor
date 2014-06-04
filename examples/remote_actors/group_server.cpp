/******************************************************************************\
 * This example program represents a minimal IRC-like group                   *
 * communication server.                                                      *
 *                                                                            *
 * Setup for a minimal chat between "alice" and "bob":                        *
 * - ./build/bin/group_server -p 4242                                         *
 * - ./build/bin/group_chat -g remote:chatroom@localhost:4242 -n alice        *
 * - ./build/bin/group_chat -g remote:chatroom@localhost:4242 -n bob          *
\******************************************************************************/

#include <string>
#include <iostream>

#include "boost/program_options.hpp"

#include "boost/actor/all.hpp"
#include "boost/actor_io/all.hpp"

using namespace std;
using namespace boost::actor;
using namespace boost::actor_io;
using namespace boost::program_options;

int main(int argc, char** argv) {
    uint16_t port = 0;
    options_description desc("Allowed options");
    desc.add_options()
        ("port,p", value<uint16_t>(&port), "set port")
        ("help,h", "print help")
    ;
    variables_map vm;
    store(parse_command_line(argc, argv, desc), vm);
    if (vm.count("help")) {
        cout << desc << endl;
        return 1;
    }
    if (port <= 1024) {
        cerr << "*** no port > 1024 given" << endl;
        return 2;
    }
    try {
        // try to bind the group server to the given port,
        // this allows other nodes to access groups of this server via
        // group::get("remote", "<group>@<host>:<port>");
        // note: it is not needed to explicitly create a <group> on the server,
        //       as groups are created on-the-fly on first usage
        publish_local_groups(port);
    }
    catch (bind_failure& e) {
        // thrown if <port> is already in use
        cerr << "*** bind_failure: " << e.what() << endl;
        return 2;
    }
    catch (network_error& e) {
        // thrown on errors in the socket API
        cerr << "*** network error: " << e.what() << endl;
        return 2;
    }
    cout << "type 'quit' to shutdown the server" << endl;
    string line;
    while (getline(cin, line)) {
        if (line == "quit") return 0;
        else cout << "illegal command" << endl;
    }
    shutdown();
}
