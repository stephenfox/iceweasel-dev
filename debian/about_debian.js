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
 * The Original Code is AboutDebian.
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

const nsISupports = Components.interfaces.nsISupports;
const nsIAboutModule = Components.interfaces.nsIAboutModule;

// You can change these if you like
const CLASS_ID = Components.ID("1359a506-95b6-4fec-9f03-3d81ce131fc0");
const CLASS_NAME = "about: handler for Debian and package related information";
const CONTRACT_ID_PREFIX = "@mozilla.org/network/protocol/about;1?what=";

// This is your constructor.
// You can do stuff here.
function AboutDebian() {
  // you can cheat and use this
  // while testing without
  // writing your own interface
  this.wrappedJSObject = this;
}

// This is the implementation of your component.
AboutDebian.prototype = {
  // for nsISupports
  QueryInterface: function(aIID)
  {
    // add any other interfaces you support here
    if (!aIID.equals(nsISupports) && !aIID.equals(nsIAboutModule))
        throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  },

  newChannel: function(uri)
  {
    var ioService = Components.classes["@mozilla.org/network/io-service;1"].getService();
    ioService = ioService.QueryInterface(Components.interfaces.nsIIOService);
    var request = uri.path.toLowerCase();
    var redirect;
    if (request == "readme.debian") {
      redirect = "file:///usr/share/doc/iceweasel/README.Debian";
    } else if (request == "bugs") {
      redirect = "http://bugs.debian.org/src:iceweasel";
    } else {
      redirect = "http://debian.org/";
    }
    var uri = ioService.newURI(redirect, null, null);
    return ioService.newChannelFromURI(uri);
  }
}

//=================================================
// Note: You probably don't want to edit anything
// below this unless you know what you're doing.
//
// Factory
var AboutDebianFactory = {
  createInstance: function (aOuter, aIID)
  {
    if (aOuter != null)
      throw Components.results.NS_ERROR_NO_AGGREGATION;
    return (new AboutDebian()).QueryInterface(aIID);
  }
};

// Module
var AboutDebianModule = {
  registerSelf: function(aCompMgr, aFileSpec, aLocation, aType)
  {
    aCompMgr = aCompMgr.QueryInterface(Components.interfaces.nsIComponentRegistrar);
    aCompMgr.registerFactoryLocation(CLASS_ID, CLASS_NAME, CONTRACT_ID_PREFIX + "readme.debian", aFileSpec, aLocation, aType);
    aCompMgr.registerFactoryLocation(CLASS_ID, CLASS_NAME, CONTRACT_ID_PREFIX + "bugs", aFileSpec, aLocation, aType);
    aCompMgr.registerFactoryLocation(CLASS_ID, CLASS_NAME, CONTRACT_ID_PREFIX + "debian", aFileSpec, aLocation, aType);
  },

  unregisterSelf: function(aCompMgr, aLocation, aType)
  {
    aCompMgr = aCompMgr.QueryInterface(Components.interfaces.nsIComponentRegistrar);
    aCompMgr.unregisterFactoryLocation(CLASS_ID_PREFIX + "readme.debian", aLocation);        
    aCompMgr.unregisterFactoryLocation(CLASS_ID_PREFIX + "bugs", aLocation);
    aCompMgr.unregisterFactoryLocation(CLASS_ID_PREFIX + "debian", aLocation);        
  },
  
  getClassObject: function(aCompMgr, aCID, aIID)
  {
    if (!aIID.equals(Components.interfaces.nsIFactory))
      throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

    if (aCID.equals(CLASS_ID))
      return AboutDebianFactory;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  },

  canUnload: function(aCompMgr) { return true; }
};

//module initialization
function NSGetModule(aCompMgr, aFileSpec) { return AboutDebianModule; }
