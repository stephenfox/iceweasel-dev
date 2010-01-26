/* ***** BEGIN LICENSE BLOCK *****
 *   Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is debAbout.
 *
 * The Initial Developer of the Original Code is
 * Mike Hommey.
 * Portions created by the Initial Developer are Copyright (C) 2007
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

var debAboutURLs = {
  get urls() {
    if (!this._urls)
      this._init();

    return this._urls;
  },

  _init: function() {
    this._urls = { "debian": "http://debian.org/" };

    var prefService = Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch);
    try {
      /* Assuming nobody change these values */
      var name = prefService.getCharPref("debian.package.name");
      var source = prefService.getCharPref("debian.package.source");
    } catch(e) {}

    if (name) {
      var file = Cc['@mozilla.org/file/local;1'].createInstance(Ci.nsILocalFile);
      file.initWithPath("/usr/share/doc/" + name + "/README.Debian.html");
      if (!file.exists()) {
        file.initWithPath("/usr/share/doc/" + name + "/README.Debian");
      }
      if (file.exists()) {
        this._urls["readme.debian"] = "file://" + file.path;
      }
      this._urls["bugs"] = "http://bugs.debian.org/" + (source ? "src:" + source : name);
    }
  }
}

function debAboutRedirector() {}

debAboutRedirector.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIAboutModule]),

  newChannel: function(uri)
  {
    var ioService = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
    var request = uri.path.toLowerCase();
    var uri = ioService.newURI(debAboutURLs.urls[request], null, null);
    var channel = ioService.newChannelFromURI(uri);
    var securityManager = Cc["@mozilla.org/scriptsecuritymanager;1"].getService(Ci.nsIScriptSecurityManager);
    var principal = securityManager.getCodebasePrincipal(uri);
    channel.owner = principal;
    return channel;
  },

  getURIFlags: function(uri)
  {
    return Ci.nsIAboutModule.URI_SAFE_FOR_UNTRUSTED_CONTENT;
  }
}

// Factory
var debAboutFactory = XPCOMUtils._getFactory(debAboutRedirector);

function debAboutComponent(key) {
  var component = function() {};
  component.prototype = {
    classDescription: "about: handler for Debian and package related information",
    classID: Components.ID("{1359a506-95b6-4fec-9f03-3d81ce131fc0}"),
    contractID: "@mozilla.org/network/protocol/about;1?what=" + key,
    _xpcom_factory: debAboutFactory
  };
  return component;
}

var NSGetModule = XPCOMUtils.generateNSGetModule([debAboutComponent(key) for (key in debAboutURLs.urls)]);
