const Cc = Components.classes;
const Ci = Components.interfaces;
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

function dumper() {}
dumper.prototype = {
  close: function() { },
  writeString: function(str) {
    dump(str);
  }
};

function rdfvalue(rdf, ds, id, prop) {
  var res = rdf.GetResource("urn:mozilla:item:" + id);
  var propres = rdf.GetResource("http://www.mozilla.org/2004/em-rdf#"+prop);
  var target = ds.GetTarget(res, propres, true);
  if (target instanceof Ci.nsIRDFLiteral)
    return target.Value;
  if (target instanceof Ci.nsIRDFResource)
    return target.Value;
  return undefined;
}

function dump_extensions(out) {
  var em = Cc["@mozilla.org/extensions/manager;1"]
           .getService(Ci.nsIExtensionManager);
  var ds = em.datasource;
  var rdf = Cc["@mozilla.org/rdf/rdf-service;1"]
            .getService(Ci.nsIRDFService);

  em.sortTypeByProperty(Ci.nsIUpdateItem.TYPE_ANY, "name", true);
  var extensions = em.getItemList(Ci.nsIUpdateItem.TYPE_ANY, { });
  out.writeString("-- Extensions information\n");
  for (var i = 0; i < extensions.length; i++) {
    var extension = extensions[i];
    var res = rdf.GetResource("urn:mozilla:item:" + extension.id);
    out.writeString("Name: " + extension.name + "\n");
    var location = em.getInstallLocation(extension.id)
                     .getItemLocation(extension.id);
    if (extension.installLocationKey == "app-profile")
      out.writeString("Location: ${PROFILE_EXTENSIONS}/" +
                      location.leafName + "\n");
    else
      out.writeString("Location: " + location.path + "\n");

    out.writeString("Status: " +
           (rdfvalue(rdf, ds, extension.id, "appDisabled") == "true" ?
                 "app-disabled" :
           (rdfvalue(rdf, ds, extension.id, "userDisabled") == "true" ?
                 "user-disabled" : "enabled")) + "\n")
    out.writeString("\n");
  }
}

function dump_plugins(out) {
  var phs = Cc["@mozilla.org/plugin/host;1"]
            .getService(Ci.nsIPluginHost);
  var plugins = phs.getPluginTags({ });
  plugins.sort();
  out.writeString("-- Plugins information\n");
  for (var i = 0; i < plugins.length; i++) {
    var plugin = plugins[i];
    out.writeString("Name: " + plugin.name +
           (plugin.version ? " (" + plugin.version + ")" : "") + "\n");
    out.writeString("Location: " + plugin.filename+"\n");
    out.writeString("Status: " + (plugin.disabled ? "disabled" : "enabled") +
                    (plugin.blocklisted ? " blocklisted" : "") + "\n");
    out.writeString("\n");
  }
}

function addonsInfoHandler() {}
addonsInfoHandler.prototype = {
  handle: function clh_handle(cmdLine) {
    var path;
    var out;
    try {
      path = cmdLine.handleFlagWithParam("dump-addons-info", false);
      if (!path)
        return;
    } catch(e) {
      if (!cmdLine.handleFlag("dump-addons-info", false))
        return;
    }

    cmdLine.preventDefault = true;

    if (path) {
      var file = Cc["@mozilla.org/file/local;1"]
                 .createInstance(Ci.nsILocalFile);
      file.initWithPath(path);
      var outstream = Cc["@mozilla.org/network/file-output-stream;1"]
                      .createInstance(Ci.nsIFileOutputStream);
      outstream.init(file, 0x2A /* TRUNCATE | WRONLY | CREATE */, 0666, 0);
      out = Cc["@mozilla.org/intl/converter-output-stream;1"]
            .createInstance(Ci.nsIConverterOutputStream);
      out.init(outstream, "UTF-8", 0, 0);
    } else
      out = new dumper();

    try {
      dump_extensions(out);
    } catch (e) {}
    try {
      dump_plugins(out);
    } catch (e) {}

    out.close();
  },

  classDescription: "addonsInfoHandler",
  classID: Components.ID("{17a1f091-70f7-411c-a9d7-191689552d01}"),
  contractID: "@mozilla.org/toolkit/addonsInfo-clh;1",
  QueryInterface: XPCOMUtils.generateQI([Ci.nsICommandLineHandler]),
  _xpcom_categories: [{category: "command-line-handler", entry: "a-addons-info"}]
};

var NSGetModule = XPCOMUtils.generateNSGetModule([addonsInfoHandler]);
