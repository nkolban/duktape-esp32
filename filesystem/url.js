/**
 * Work with URLs.
 * References:
 * * Article on using Regex for JS URL pasrsing: http://stackoverflow.com/questions/27745/getting-parts-of-a-url-regex
 * * Node.js URL docs
 * * https://tools.ietf.org/html/rfc3986#appendix-B
 * http://stackoverflow.com/questions/6168260/how-to-parse-a-url
 * 
 * const URL = require("url");
 * var obj = URL.parse("http://www.google.com");
 */

/* globals module */
/*
 * http://www.google.com:123/p/q?a=b&x=y
 * -> [http://www.google.com:123/p/q?a=b&x=y, http:, http, //www.google.com:123, www.google.com:123, /p/q, ?a=b&x=y, a=b&x=y, undefined, undefined]
 * ->                    0                      1      2            3                   4              5         6     7    
 */
var urlRegexpString = "^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\\?([^#]*))?(#(.*))?";
var urlRegexp = new RegExp(urlRegexpString, 'i');
var queryRegexp = new RegExp("([^?=&]+)(=([^&]*))?", "g");

module.exports = {
	/*
	 * Parse a URL string into its constituent parts.  An object is returned which contains:
	 * {
	 *    host:     <The host part of the URL>
	 *    hostname: <The hostname part of the URL; no port number>
	 *    href:     <The original URL>
	 *    pathname: <The pathname of the URL>
    *    port:     <The port number>    
	 *    protocol: <The protocol>
	 *    query:    <An object with the query parts broken out>
	 *    search:   <The search part of the URL including the ?>
	 * }
	 */
	parse: function(urlString) {
		var parts = urlString.match(urlRegexp);
		var retObj = {
			host: parts[4],
			href: urlString,
			protocol: parts[1],
			pathname: parts[5],
			search: parts[6],
			query: {}
		};
		var hostPortParts = retObj.host.split(":");
		retObj.hostname = hostPortParts[0];
		if (hostPortParts.length == 1) {
			retObj.port = "80";
		} else {
			retObj.port = hostPortParts[1];
		}
		urlString.replace(
		   queryRegexp,
			function($0, $1, $2, $3) { retObj.query[$1] = $3; }
		);
		return retObj;
	}, // parse
	
	/*
	 * Parse a query string and return an object containing its name/value pairs.
	 * In this implementation we use a simple brute force but ideally we will 
	 * use a RegExp.
	 */
	queryParse: function(queryString) {
		var parts = decodeURIComponent(queryString).split("&");
		var result = {};
		var i;
		for (i=0; i<parts.length; i++) {
			var nameValue = parts[i].split("=");
			result[nameValue[0]] = nameValue[1];
		}
		return result;
	} // queryParse
};