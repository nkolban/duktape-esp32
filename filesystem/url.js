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
	 *    host: <The host part of the URL>
	 *    href: <The original URL>
	 *    protocol: <The protocol>
	 *    pathname: <The pathname of the URL>
	 *    search: <The search part of the URL including the ?>
	 *    query: <An object with the query parts broken out>
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
		urlString.replace(
		   queryRegexp,
			function($0, $1, $2, $3) { retObj.query[$1] = $3; }
		);
		return retObj;
	}
};