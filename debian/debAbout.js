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

/* Used Javascript XPCOM component generator from
   http://ted.mielczarek.org/code/mozilla/jscomponentwiz/ to get a skeleton */

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

// You can change these if you like
const CLASS_ID = Components.ID("1359a506-95b6-4fec-9f03-3d81ce131fc0");
const CLASS_NAME = "about: handler for Debian and package related information";
const CONTRACT_ID_PREFIX = "@mozilla.org/network/protocol/about;1?what=";

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

// This is your constructor.
// You can do stuff here.
function debAboutRedirector() {
  // you can cheat and use this
  // while testing without
  // writing your own interface
  this.wrappedJSObject = this;
}

// This is the implementation of your component.
debAboutRedirector.prototype = {
  // for nsISupports
  QueryInterface: function(aIID)
  {
    // add any other interfaces you support here
    if (!aIID.equals(Ci.nsISupports) && !aIID.equals(Ci.nsIAboutModule))
        throw Cr.NS_ERROR_NO_INTERFACE;
    return this;
  },

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

//=================================================
// Note: You probably don't want to edit anything
// below this unless you know what you're doing.
//
// Factory
var debAboutFactory = {
  createInstance: function (aOuter, aIID)
  {
    if (aOuter != null)
      throw Cr.NS_ERROR_NO_AGGREGATION;
    return (new debAboutRedirector()).QueryInterface(aIID);
  }
};

// Module
var debAboutModule = {
  registerSelf: function(aCompMgr, aFileSpec, aLocation, aType)
  {

    aCompMgr = aCompMgr.QueryInterface(Ci.nsIComponentRegistrar);
    for (var key in debAboutURLs.urls) {
      aCompMgr.registerFactoryLocation(CLASS_ID, CLASS_NAME, CONTRACT_ID_PREFIX + key, aFileSpec, aLocation, aType);
    }
  },

  unregisterSelf: function(aCompMgr, aLocation, aType)
  {
    aCompMgr = aCompMgr.QueryInterface(Ci.nsIComponentRegistrar);
    for (var key in this.urls) {
      aCompMgr.unregisterFactoryLocation(CLASS_ID_PREFIX + key, aLocation);
    }
  },

  getClassObject: function(aCompMgr, aCID, aIID)
  {
    if (!aIID.equals(Ci.nsIFactory))
      throw Cr.NS_ERROR_NOT_IMPLEMENTED;

    if (aCID.equals(CLASS_ID))
      return debAboutFactory;

    throw Cr.NS_ERROR_NO_INTERFACE;
  },

  canUnload: function(aCompMgr) { return true; }
};

//module initialization
function NSGetModule(aCompMgr, aFileSpec) { return debAboutModule; }
