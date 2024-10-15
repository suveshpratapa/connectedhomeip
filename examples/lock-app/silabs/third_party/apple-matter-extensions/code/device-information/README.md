# Apple Device Information cluster

To add support for the Apple Device Information cluster in your
application using the [Matter
SDK](https://github.com/project-chip/connectedhomeip/), perform the
following steps while in the root of the Matter repository:

1. Place the [`apple-device-information-cluster.xml`](apple-device-information-cluster.xml) file in this directory at `src/app/zap-templates/zcl/data-model/apple/apple-device-information-cluster.xml`

2. Modify `src/app/zap-templates/zcl/zcl.json` to include the string `"./data-model/apple/"` in the `xmlRoot` array.

3. Modify `src/app/zap-templates/zcl/zcl.json` to include the string `"apple-device-information-cluster.xml"` in the `xmlFile` array.

4. Run `scripts/tools/zap/generate.py src/controller/data_model/controller-clusters.zap -t src/app/common/templates/templates.json -o zzz_generated/app-common/app-common/zap-generated` to generate the app-common files for the extension.

5. Modify `src/app/zap_cluster_list.json` to include `"APPLE_DEVICE_INFORMATION_CLUSTER": [ "apple-device-information-server" ]` in the `"ServerDirectories"` section.

6. Place the [`apple-device-information-server.cpp`](apple-device-information-server.cpp) file in this directory at `src/app/clusters/apple-device-information-server/apple-device-information-server.cpp`.

7. Run `scripts/tools/zap/run_zaptool.sh examples/lock-app/lock-common/lock-app.zap` (or whichever lock-app `.zap` file is being used) and:
   1. Enable the server side of the "Apple Device Information" cluster on endpoint 0.
   2. Enable the SupportsTapToUnlock attribute in that cluster.
   3. Set the default value of that attribute to 1.

8. Run `scripts/tools/zap/generate.py examples/lock-app/lock-common/lock-app.zap` (or whichever lock-app `.zap` file is being used) to generate the files for the lock app.

9. Run the `gn gen` command to generate files.

10. Run the `ninja` command to compile the generated files from `gn`.

Steps 1 through 8 may be accomplished by running the
[`update-matter-repo.py`](update-matter-repo.py) script in this directory
while at the root of the Matter repository.
