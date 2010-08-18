/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Mike Hommey <mh@glandium.org>.
 * Portions created by the Initial Developer are Copyright (C) 2010
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
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

#include "nsWeakReference.h"
#include "debGIOFileMonitorService.h"
#include "nsIGenericFactory.h"
#include "nsStringAPI.h"
#include "nsILocalFile.h"

extern "C" {
/* #pragma are only here because I don't want to patch build/system-headers
   while this component lives in debian/ */
#pragma GCC visibility push(default)
#include <gio/gio.h>
#pragma GCC visibility pop
}

NS_GENERIC_FACTORY_CONSTRUCTOR(debGIOFileMonitorService)

static const nsModuleComponentInfo components[] = {
  { "GIO File Monitor Service",
    DEB_GIOFILEMONITORSERVICE_CID,
    DEB_GIOFILEMONITORSERVICE_CONTRACTID,
    debGIOFileMonitorServiceConstructor }
};

NS_IMPL_NSGETMODULE(giofilemonitor, components)

NS_IMPL_ISUPPORTS1(debGIOFileMonitorService, debIGIOFileMonitorService)

static void file_changed(GFileMonitor *aMonitor,
                         GFile *aFile,
                         GFile *aOther_file,
                         GFileMonitorEvent aEvent_type,
                         debIGIOFileMonitorObserver *aObserver)
{
  /* aOther_file is ignored, it doesn't seem to be ever used.
   * We also don't support G_FILE_MONITOR_EVENT_PRE_UNMOUNT and
   * G_FILE_MONITOR_EVENT_UNMOUNTED events. */
  if (!aObserver || !aFile || (aEvent_type >= G_FILE_MONITOR_EVENT_PRE_UNMOUNT))
    return;

  nsresult rv;
  nsCOMPtr<debIGIOFileMonitorObserver> observer = do_QueryInterface(aObserver, &rv);
  if (NS_FAILED(rv)) return;

  nsCOMPtr<nsILocalFile> file;
  char *path = g_file_get_path(aFile);
  rv = NS_NewNativeLocalFile(nsDependentCString(path), PR_FALSE, getter_AddRefs(file));
  g_free(path);
  if (NS_FAILED(rv)) return;

  observer->Notify(file, (debGIOFileMonitorEvent) aEvent_type);
}

NS_IMETHODIMP
debGIOFileMonitorService::Monitor(nsIFile *aFile, debIGIOFileMonitorObserver *aObserver)
{
  if (!aFile || !aObserver)
    return NS_ERROR_FAILURE;

  nsresult rv;
  nsCAutoString path;
  rv = aFile->GetNativePath(path);
  NS_ENSURE_SUCCESS(rv, rv);

  if (path.IsEmpty())
    return NS_ERROR_FAILURE;

  GFile *monitored_file = g_file_new_for_path(path.get());
  if (!monitored_file)
    return NS_ERROR_FAILURE;

  GFileMonitor *monitor = g_file_monitor(monitored_file, G_FILE_MONITOR_NONE, NULL, NULL);
  g_object_unref(monitored_file);

  if (!monitor)
    return NS_ERROR_FAILURE;

  NS_ENSURE_SUCCESS(rv, rv);

  /* Keep a reference of the observer */
  aObserver->AddRef();
  g_signal_connect(monitor, "changed", G_CALLBACK(file_changed), aObserver);

  return NS_OK;
}
