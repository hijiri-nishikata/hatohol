/*
 * Copyright (C) 2013 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Hatohol. If not, see <http://www.gnu.org/licenses/>.
 */

if ( "undefined" == typeof lib ) {
  var lib = require("../static/js/library");
}

exports.parseData = function (rd) {
  var pd = new Object();
  var server, host, item;
  var x;
  var o;
  var ss, is, hs;

  pd.hosts  = new Object();
  pd.values = new Object();

  ss = [];
  is = [];
  hs = new Object();
  for (x = 0; x < rd["items"].length; ++x) {
    o = rd["items"][x];
    server = rd["servers"][o["serverId"]]["name"];
    host   = o["hostId"]; // rd["servers"][o["serverId"]]["hosts"][o["hostId"]]["name"];  ITS NOT IMPLEMENTED YET
    item   = o["brief"];

    ss.push(server);
    is.push(item);
    if ( !(server in hs) ) {
      hs[server] = [];
    }
    hs[server].push(host);

    if ( !(server in pd.values) ) {
      pd.values[server] = new Object();
    }
    if ( !(host in pd.values[server]) ) {
      pd.values[server][host] = new Object();
    }

    if ( !(item in pd.values[server][host]) ) {
      pd.values[server][host][item] = o;
    }
  }

  pd.servers = ss.uniq().sort();
  pd.items   = is.uniq().sort();
  for ( server in hs ) {
    pd.hosts[server] = hs[server].uniq().sort();
  }

  return pd;
}

exports.drawTableHeader = function (pd) {
  var klass, server, host;
  var x;
  var hs;
  var s1, s2;

  s1 = "<tr><th></th>";
  s2 = "<tr><th></th>";
  for ( server in pd.hosts ) {
    hs = pd.hosts[server];
    s1 += "<td style='text-align: center' class='" + server + "' colspan='" + hs.length + "'>" + server + "</td>";
    for (x = 0; x < hs.length; ++x) {
      host  = hs[x];
      klass = server + " " + host;
      s2 += "<td class='" + klass + "'>" + host + "</td>";
    }
  }
  s1 += "</tr>";
  s2 += "</tr>";

  return s1 + s2;
}

exports.drawTableBody = function (pd) {
  var klass, item, server, host, severity;
  var x, y;
  var o;
  var hs;
  var s;

  s = "";
  for (y = 0; y < pd.items.length; ++y) {
    item = pd.items[y];
    s += "<tr>";
    s += "<th>" + item + "</th>";
    for ( server in pd.hosts ) {
      hs = pd.hosts[server];
      for (x = 0; x < hs.length; ++x) {
        host  = hs[x];
        klass = server + " " + host;
        if ( item in pd.values[server][host] ) {
          o = pd.values[server][host][item];
          severity = "";  // TODO: build severity information, from somewhere :-)
          if ( "undefined" != typeof o["lastValue"] ) {
            s += "<td class='" + klass + " " + severity + "'>" + o["lastValue"] + "</td>";
          }
          else {
            s += "<td class='" + klass + " " + severity + "'>&nbsp;</td>";
          }
        }
        else {
          s += "<td class='" + klass + "'>&nbsp;</td>";
        }
      }
    }
    s += "</tr>";
  }

  return s;
}
