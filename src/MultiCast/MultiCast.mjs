import dgram from 'dgram';

// push Query to : the multicast pool(group)
const multicast_address = "239.255.0.1";
const multicast_port = 30003;

const listen_address = "0.0.0.0";
// random
const listen_port = 30003;

let socket = dgram.createSocket({
    type: 'udp4',
    reuseAddr: true // for testing multiple instances on localhost
});

socket.bind(listen_port, listen_address);

socket.on('message', (msg, remote) => {
    console.log('message: ', msg.toString().trim(), " from ", `${remote.address}:${remote.port}`);
});

socket.on("listening", function () {
    socket.setBroadcast(true);
    socket.setMulticastTTL(128);
    socket.addMembership(multicast_address);
    console.log('Multicast listening . . . ');
});

;(() => {
    setInterval(() => {
        let message = JSON.stringify({MultiCast: "Query"})
        socket.send(message, 0, message.length, multicast_port, multicast_address)
    }, 5000);
})();
