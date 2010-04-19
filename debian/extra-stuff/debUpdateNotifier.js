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
 * Portions created by the Initial Developer are Copyright (C) 2010
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

function fileAppend(file, leaf) {
  file.append(leaf);
  return file;
}

function setResult(result, file, type) {
  try {
    file.normalize();
  } catch(e) { }
  result[file.path] = type;
}

function monitoredList() {
  var result = {};
  var app = Cc["@mozilla.org/xre/app-info;1"]
            .getService(Ci.nsIXULAppInfo).QueryInterface(Ci.nsIXULRuntime);
  var ds = Cc["@mozilla.org/file/directory_service;1"]
           .getService(Ci.nsIProperties);

  // $GREDIR/platform.ini
  var gredir = ds.get("GreD", Ci.nsIFile);
  setResult(result, gredir, "gre");

  // $APPDIR/application.ini if $APPDIR != $GREDIR
  var dir = ds.get("CurProcD", Ci.nsIFile);
  if (!dir.equals(gredir))
    setResult(result, dir, "app");

  // $APPDIR/extensions
  setResult(result, fileAppend(dir, "extensions"), "extdir");

  // system /usr/share extensions directory
  dir = ds.get("XRESysSExtPD", Ci.nsIFile);
  setResult(result, fileAppend(dir, app.ID), "extdir");

  // system /usr/lib extensions directory
  dir = ds.get("XRESysLExtPD", Ci.nsIFile);
  setResult(result, fileAppend(dir, app.ID), "extdir");

  // plugins directories
  var userplugins = ds.get("UserPlugins", Ci.nsIFile);
  dir = ds.get("APluginsDL", Ci.nsISimpleEnumerator);
  while (dir.hasMoreElements()) {
    var plugindir = dir.getNext().QueryInterface(Ci.nsIFile);
    if (!userplugins || !plugindir.equals(userplugins))
      setResult(result, plugindir, "plugdir");
  }

  // individual plugins
  var phs = Cc["@mozilla.org/plugin/host;1"]
            .getService(Ci.nsIPluginHost);
  var plugins = phs.getPluginTags({ });
  for (var i = 0; i < plugins.length; i++) {
    var file = Cc["@mozilla.org/file/local;1"]
               .createInstance(Ci.nsILocalFile);
    var plugin = plugins[i];
    if (Ci.nsIPluginTag_1_9_2)
      plugin = plugin.QueryInterface(Ci.nsIPluginTag_1_9_2);
    file.initWithPath(plugin.fullpath ? plugin.fullpath
                                      : plugin.filename);
    if (!userplugins || !file.parent.equals(userplugins))
      setResult(result, file, "plugin");
  }

  // individual extensions
  var em = Cc["@mozilla.org/extensions/manager;1"]
           .getService(Ci.nsIExtensionManager);
  var extensions = em.getItemList(Ci.nsIUpdateItem.TYPE_ANY, { });
  for (var i = 0; i < extensions.length; i++) {
    var extension = extensions[i];
    if (extension.installLocation != "app-profile" &&
        extension.installLocation != "app-system-user") {
      var location = em.getInstallLocation(extension.id)
                       .getItemLocation(extension.id);
      setResult(result, location, "ext");
    }
  }

  return result;
}

function fmObserver() {}

fmObserver.prototype = {
  notify: function(file, event) {
    if (this.notified)
      return;
    this.notified = true;
    if (!this.list)
      this.list = monitoredList();
    monitoredfile = file;
    if (!this.list[monitoredfile.path])
      monitoredfile = file.parent;
    if (this.list[monitoredfile.path]) {
      var name;
      var sbs = Cc["@mozilla.org/intl/stringbundle;1"]
               .getService(Ci.nsIStringBundleService);
      try {
        name = sbs.createBundle("chrome://branding/locale/brand.properties")
               .GetStringFromName("brandShortName");
      } catch(e) {
        name = Cc["@mozilla.org/xre/app-info;1"]
               .getService(Ci.nsIXULAppInfo)
               .QueryInterface(Ci.nsIXULRuntime).name;
      }
      var dun = sbs.createBundle("chrome://debian/locale/debUpdateNotifier.properties");
      var title = dun.GetStringFromName("title");
      var text = dun.formatStringFromName("message", [name], 1);

      if (!Cc["@mozilla.org/embedcomp/prompt-service;1"]
           .getService(Ci.nsIPromptService)
           .confirm(null, title, text))
        return;

      var os = Cc["@mozilla.org/observer-service;1"]
               .getService(Ci.nsIObserverService);
      var cancelQuit = Cc["@mozilla.org/supports-PRBool;1"]
                       .createInstance(Ci.nsISupportsPRBool);
      os.notifyObservers(cancelQuit, "quit-application-requested", "restart");

      // Something aborted the quit process.
      if (cancelQuit.data)
        return;

      Cc["@mozilla.org/toolkit/app-startup;1"].getService(Ci.nsIAppStartup)
      .quit(Ci.nsIAppStartup.eRestart | Ci.nsIAppStartup.eAttemptQuit);
    }
  }
}

function debUpdateNotifier() {}

debUpdateNotifier.prototype = {
  classDescription: "Debian system updates notifier",
  classID: Components.ID("{7ff849af-8344-4784-a575-09c172ff8b72}"),
  contractID: "@debian.org/update-notifier;1",
  _xpcom_categories: [{category: "app-startup", service: true}],
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference]),
  observe: function (subject, topic, data) {
    if (topic == "app-startup") {
      var os = Cc["@mozilla.org/observer-service;1"]
               .getService(Ci.nsIObserverService);
      os.addObserver(this, "final-ui-startup", true);
    }
    if (topic != "final-ui-startup") return;
    var fmo = new fmObserver();
    var fm = Cc["@debian.org/gio-file-monitor-service;1"]
             .getService(Ci.debIGIOFileMonitorService);

    var list = monitoredList();
    for (var path in list) {
      var file = Cc["@mozilla.org/file/local;1"]
                 .createInstance(Ci.nsILocalFile);
      file.initWithPath(path);
      // We don't want to monitor e.g. /usr/lib/mozilla/plugins/libjavaplugin.so
      // when we already monitor /usr/lib/mozilla/plugins.
      // But we want to monitor subdirectories.
      if (list[path].substr(-3) == "dir" || ! list[file.parent.path])
        fm.monitor(file, fmo);
    }
  }
}

var NSGetModule = XPCOMUtils.generateNSGetModule([debUpdateNotifier]);
