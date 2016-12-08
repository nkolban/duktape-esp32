console.log("Hello World\n");var sockfd = OS.socket().sockfd;
console.log("sockfd = " + sockfd + "\n");
OS.bind({port: 8887, sockfd: sockfd});
OS.listen({sockfd: sockfd});
/*
while(true) {
    var selectData = OS.select({readfds: [sockfd], writefds: [], exceptfds: []});
    console.log(JSON.stringify(selectData) + "\n");
}
*/

ret = OS.accept({sockfd: sockfd});
console.log(ret.sockfd);
setInterval(function() {
    var selectData = OS.select({readfds: [sockfd], writefds: [], exceptfds: []});
    console.log(JSON.stringify(selectData) + "\n");
    if (selectData.readfds.length > 0) {
        ret = OS.accept({sockfd: selectData.readfds[0]});
    }
}, 3000);

//OS.close({sockfd: sockfd});
//OS.close({sockfd: ret.sockfd});

/*
setInterval(function() {
    var selectData = OS.select({readfds: [sockfd], writefds: [], exceptfds: []});
    console.log(JSON.stringify(selectData) + "\n");  
}, 3000);
*/
console.log("Done!\n");