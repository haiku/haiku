(Copied mostly from <http://urnenfeld.blogspot.de/2012/07/in-past-i-got-to-know-that-motivation.html>)

**L2cap under `network/protocols/l2cap`**: Provides socket interface to have l2cap channels. L2CAP offers connection oriented and connectionless sockets. But bluetooth stack as this point has no interchangeability with TCP/IP, A Higher level Bluetooth profile must be implemented

**HCI under `src/add-ons/kernel/bluetooth`**: Here we have 2 modules, one for handling global bluetooth data structures such as connection handles and L2cap channels, and frames

**H2generic under `src/add-ons/kernel/drivers/bluetooth`**: The USB driver, implementing the H2 transport.

**Bluetooth kit under `src/kit/bluetooth`**: C++ implementation based on JSR82 api.

**Bluetooth Server under `src/servers/bluetooth`**: Basically handling opened devices (local connected fisically in our system) and forwaring kit calls to them.

**Bluetooth Preferences under `src/preferences/bluetooth`**: Configuration using the kit

**Test applications under `src/tests/kits/bluetooth`**.

There is a small prototype component which is not here documented below  src/add-ons/bluetooth/ResetLocalDevice. Its intention was to be an add-on of bluetooth preferences, So that new HCI commands could be customized by users or external developers. I did not like at the end the idea, I did not find the flexibility I wanted.
