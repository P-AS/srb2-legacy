
  var Module = typeof Module != 'undefined' ? Module : {};

  if (!Module['expectedDataFileDownloads']) Module['expectedDataFileDownloads'] = 0;
  Module['expectedDataFileDownloads']++;
  (() => {
    // Do not attempt to redownload the virtual filesystem data when in a pthread or a Wasm Worker context.
    var isPthread = typeof ENVIRONMENT_IS_PTHREAD != 'undefined' && ENVIRONMENT_IS_PTHREAD;
    var isWasmWorker = typeof ENVIRONMENT_IS_WASM_WORKER != 'undefined' && ENVIRONMENT_IS_WASM_WORKER;
    if (isPthread || isWasmWorker) return;
    var isNode = globalThis.process && globalThis.process.versions && globalThis.process.versions.node && globalThis.process.type != 'renderer';
    async function loadPackage(metadata) {

      var PACKAGE_PATH = '';
      if (typeof window === 'object') {
        PACKAGE_PATH = window['encodeURIComponent'](window.location.pathname.substring(0, window.location.pathname.lastIndexOf('/')) + '/');
      } else if (typeof process === 'undefined' && typeof location !== 'undefined') {
        // web worker
        PACKAGE_PATH = encodeURIComponent(location.pathname.substring(0, location.pathname.lastIndexOf('/')) + '/');
      }
      var PACKAGE_NAME = 'timidity.data';
      var REMOTE_PACKAGE_BASE = 'timidity.data';
      var REMOTE_PACKAGE_NAME = Module['locateFile'] ? Module['locateFile'](REMOTE_PACKAGE_BASE, '') : REMOTE_PACKAGE_BASE;
      var REMOTE_PACKAGE_SIZE = metadata['remote_package_size'];

      async function fetchRemotePackage(packageName, packageSize) {
        if (isNode) {
          var contents = require('fs').readFileSync(packageName);
          return new Uint8Array(contents).buffer;
        }
        if (!Module['dataFileDownloads']) Module['dataFileDownloads'] = {};
        try {
          var response = await fetch(packageName);
        } catch (e) {
          throw new Error(`Network Error: ${packageName}`, {e});
        }
        if (!response.ok) {
          throw new Error(`${response.status}: ${response.url}`);
        }

        const chunks = [];
        const headers = response.headers;
        const total = Number(headers.get('Content-Length') || packageSize);
        let loaded = 0;

        Module['setStatus'] && Module['setStatus']('Downloading data...');
        const reader = response.body.getReader();

        while (1) {
          var {done, value} = await reader.read();
          if (done) break;
          chunks.push(value);
          loaded += value.length;
          Module['dataFileDownloads'][packageName] = {loaded, total};

          let totalLoaded = 0;
          let totalSize = 0;

          for (const download of Object.values(Module['dataFileDownloads'])) {
            totalLoaded += download.loaded;
            totalSize += download.total;
          }

          Module['setStatus'] && Module['setStatus'](`Downloading data... (${totalLoaded}/${totalSize})`);
        }

        const packageData = new Uint8Array(chunks.map((c) => c.length).reduce((a, b) => a + b, 0));
        let offset = 0;
        for (const chunk of chunks) {
          packageData.set(chunk, offset);
          offset += chunk.length;
        }
        return packageData.buffer;
      }

    async function runWithFS(Module) {

      function assert(check, msg) {
        if (!check) throw new Error(msg);
      }
Module['FS_createPath']("/", "instruments", true, true);

    for (var file of metadata['files']) {
      var name = file['filename']
      Module['addRunDependency'](`fp ${name}`);
    }

        var PACKAGE_UUID = metadata['package_uuid'];
        var IDB_RO = "readonly";
        var IDB_RW = "readwrite";
        var DB_NAME = "EM_PRELOAD_CACHE";
        var DB_VERSION = 1;
        var METADATA_STORE_NAME = 'METADATA';
        var PACKAGE_STORE_NAME = 'PACKAGES';

        async function openDatabase() {
          if (typeof indexedDB == 'undefined') {
            throw new Error('using IndexedDB to cache data can only be done on a web page or in a web worker');
          }
          return new Promise((resolve, reject) => {
            var openRequest = indexedDB.open(DB_NAME, DB_VERSION);
            openRequest.onupgradeneeded = (event) => {
              var db = /** @type {IDBDatabase} */ (event.target.result);

              if (db.objectStoreNames.contains(PACKAGE_STORE_NAME)) {
                db.deleteObjectStore(PACKAGE_STORE_NAME);
              }
              var packages = db.createObjectStore(PACKAGE_STORE_NAME);

              if (db.objectStoreNames.contains(METADATA_STORE_NAME)) {
                db.deleteObjectStore(METADATA_STORE_NAME);
              }
              var metadata = db.createObjectStore(METADATA_STORE_NAME);
            };
            openRequest.onsuccess = (event) => {
              var db = /** @type {IDBDatabase} */ (event.target.result);
              resolve(db);
            };
            openRequest.onerror = reject;
          });
        }

        // This is needed as chromium has a limit on per-entry files in IndexedDB
        // https://cs.chromium.org/chromium/src/content/renderer/indexed_db/webidbdatabase_impl.cc?type=cs&sq=package:chromium&g=0&l=177
        // https://cs.chromium.org/chromium/src/out/Debug/gen/third_party/blink/public/mojom/indexeddb/indexeddb.mojom.h?type=cs&sq=package:chromium&g=0&l=60
        // We set the chunk size to 64MB to stay well-below the limit
        var CHUNK_SIZE = 64 * 1024 * 1024;

        async function cacheRemotePackage(db, packageName, packageData, packageMeta) {
          var transactionPackages = db.transaction([PACKAGE_STORE_NAME], IDB_RW);
          var packages = transactionPackages.objectStore(PACKAGE_STORE_NAME);
          var chunkSliceStart = 0;
          var nextChunkSliceStart = 0;
          var chunkCount = Math.ceil(packageData.byteLength / CHUNK_SIZE);
          var finishedChunks = 0;

          return new Promise((resolve, reject) => {
            for (var chunkId = 0; chunkId < chunkCount; chunkId++) {
              nextChunkSliceStart += CHUNK_SIZE;
              var putPackageRequest = packages.put(
                packageData.slice(chunkSliceStart, nextChunkSliceStart),
                `package/${packageName}/${chunkId}`
              );
              chunkSliceStart = nextChunkSliceStart;
              putPackageRequest.onsuccess = (event) => {
                finishedChunks++;
                if (finishedChunks == chunkCount) {
                  var transaction_metadata = db.transaction(
                    [METADATA_STORE_NAME],
                    IDB_RW
                  );
                  var metadata = transaction_metadata.objectStore(METADATA_STORE_NAME);
                  var putMetadataRequest = metadata.put(
                    {
                      'uuid': packageMeta.uuid,
                      'chunkCount': chunkCount
                    },
                    `metadata/${packageName}`
                  );
                  putMetadataRequest.onsuccess = (event) => resolve(packageData);
                  putMetadataRequest.onerror = reject;
                }
              };
              putPackageRequest.onerror = reject;
            }
          });
        }

        /*
         * Check if there's a cached package, and if so whether it's the latest available.
         * Resolves to the cached metadata, or `null` if it is missing or out-of-date.
         */
        async function checkCachedPackage(db, packageName) {
          var transaction = db.transaction([METADATA_STORE_NAME], IDB_RO);
          var metadata = transaction.objectStore(METADATA_STORE_NAME);
          var getRequest = metadata.get(`metadata/${packageName}`);
          return new Promise((resolve, reject) => {
            getRequest.onsuccess = (event) => {
              var result = event.target.result;
              if (result && PACKAGE_UUID === result['uuid']) {
                resolve(result);
              } else {
                resolve(null);
              }
            }
            getRequest.onerror = reject;
          });
        }

        async function fetchCachedPackage(db, packageName, metadata) {
          var transaction = db.transaction([PACKAGE_STORE_NAME], IDB_RO);
          var packages = transaction.objectStore(PACKAGE_STORE_NAME);

          var chunksDone = 0;
          var totalSize = 0;
          var chunkCount = metadata['chunkCount'];
          var chunks = new Array(chunkCount);

          return new Promise((resolve, reject) => {
            for (var chunkId = 0; chunkId < chunkCount; chunkId++) {
              var getRequest = packages.get(`package/${packageName}/${chunkId}`);
              getRequest.onsuccess = (event) => {
                if (!event.target.result) {
                  reject(`CachedPackageNotFound for: ${packageName}`);
                  return;
                }
                // If there's only 1 chunk, there's nothing to concatenate it with so we can just return it now
                if (chunkCount == 1) {
                  resolve(event.target.result);
                } else {
                  chunksDone++;
                  totalSize += event.target.result.byteLength;
                  chunks.push(event.target.result);
                  if (chunksDone == chunkCount) {
                    if (chunksDone == 1) {
                      resolve(event.target.result);
                    } else {
                      var tempTyped = new Uint8Array(totalSize);
                      var byteOffset = 0;
                      for (var chunkId in chunks) {
                        var buffer = chunks[chunkId];
                        tempTyped.set(new Uint8Array(buffer), byteOffset);
                        byteOffset += buffer.byteLength;
                        buffer = undefined;
                      }
                      chunks = undefined;
                      resolve(tempTyped.buffer);
                      tempTyped = undefined;
                    }
                  }
                }
              };
              getRequest.onerror = reject;
            }
          });
        }

      async function processPackageData(arrayBuffer) {
        assert(arrayBuffer, 'Loading data file failed.');
        assert(arrayBuffer.constructor.name === ArrayBuffer.name, 'bad input to processPackageData ' + arrayBuffer.constructor.name);
        var byteArray = new Uint8Array(arrayBuffer);
        var curr;
        // Reuse the bytearray from the XHR as the source for file reads.
          for (var file of metadata['files']) {
            var name = file['filename'];
            var data = byteArray.subarray(file['start'], file['end']);
            // canOwn this data in the filesystem, it is a slice into the heap that will never change
        Module['FS_createDataFile'](name, null, data, true, true, true);
        Module['removeRunDependency'](`fp ${name}`);
          }
          Module['removeRunDependency']('datafile_timidity.data');
      }
      Module['addRunDependency']('datafile_timidity.data');

      if (!Module['preloadResults']) Module['preloadResults'] = {};

        async function preloadFallback(error) {
          console.error(error);
          console.error('falling back to default preload behavior');
          processPackageData(await fetchRemotePackage(REMOTE_PACKAGE_NAME, REMOTE_PACKAGE_SIZE));
        }

        try {
          var db = await openDatabase();
          var pkgMetadata = await checkCachedPackage(db, PACKAGE_PATH + PACKAGE_NAME);
          var useCached = !!pkgMetadata;
          Module['preloadResults'][PACKAGE_NAME] = {fromCache: useCached};
          if (useCached) {
            processPackageData(await fetchCachedPackage(db, PACKAGE_PATH + PACKAGE_NAME, pkgMetadata));
          } else {
            var packageData = await fetchRemotePackage(REMOTE_PACKAGE_NAME, REMOTE_PACKAGE_SIZE);
            try {
              processPackageData(await cacheRemotePackage(db, PACKAGE_PATH + PACKAGE_NAME, packageData, {uuid:PACKAGE_UUID}))
            } catch (error) {
              console.error(error);
              processPackageData(packageData);
            }
          }
        } catch(e) {
          await preloadFallback(e);
        }

        Module['setStatus'] && Module['setStatus']('Downloading...');

    }
    if (Module['calledRun']) {
      runWithFS(Module);
    } else {
      if (!Module['preRun']) Module['preRun'] = [];
      Module['preRun'].push(runWithFS); // FS is not initialized yet, wait for it
    }

    }
    loadPackage({"files": [{"filename": "/README", "start": 0, "end": 79}, {"filename": "/copyright.txt", "start": 79, "end": 1596}, {"filename": "/instruments/acbass.pat", "start": 1596, "end": 35609}, {"filename": "/instruments/accordn.pat", "start": 35609, "end": 99680}, {"filename": "/instruments/acguitar.pat", "start": 99680, "end": 887519}, {"filename": "/instruments/acpiano.pat", "start": 887519, "end": 1786160}, {"filename": "/instruments/agogo.pat", "start": 1786160, "end": 1813887}, {"filename": "/instruments/agogohi.pat", "start": 1813887, "end": 1823752}, {"filename": "/instruments/agogolo.pat", "start": 1823752, "end": 1836769}, {"filename": "/instruments/altosax.pat", "start": 1836769, "end": 1866086}, {"filename": "/instruments/applause.pat", "start": 1866086, "end": 1926535}, {"filename": "/instruments/atmosphr.pat", "start": 1926535, "end": 1989570}, {"filename": "/instruments/aurora.pat", "start": 1989570, "end": 2052073}, {"filename": "/instruments/bagpipes.pat", "start": 2052073, "end": 2068130}, {"filename": "/instruments/banjo.pat", "start": 2068130, "end": 2132653}, {"filename": "/instruments/barisax.pat", "start": 2132653, "end": 2191296}, {"filename": "/instruments/basslead.pat", "start": 2191296, "end": 2244685}, {"filename": "/instruments/bassoon.pat", "start": 2244685, "end": 2265838}, {"filename": "/instruments/belltree.pat", "start": 2265838, "end": 2332025}, {"filename": "/instruments/bongohi.pat", "start": 2332025, "end": 2354256}, {"filename": "/instruments/bongolo.pat", "start": 2354256, "end": 2374781}, {"filename": "/instruments/bottle.pat", "start": 2374781, "end": 2463272}, {"filename": "/instruments/bowglass.pat", "start": 2463272, "end": 2512963}, {"filename": "/instruments/britepno.pat", "start": 2512963, "end": 3503668}, {"filename": "/instruments/cabasa.pat", "start": 3503668, "end": 3520871}, {"filename": "/instruments/calliope.pat", "start": 3520871, "end": 3567174}, {"filename": "/instruments/carillon.pat", "start": 3567174, "end": 3579263}, {"filename": "/instruments/castinet.pat", "start": 3579263, "end": 3591612}, {"filename": "/instruments/celeste.pat", "start": 3591612, "end": 3611819}, {"filename": "/instruments/cello.pat", "start": 3611819, "end": 3671734}, {"filename": "/instruments/charang.pat", "start": 3671734, "end": 3762395}, {"filename": "/instruments/chiflead.pat", "start": 3762395, "end": 3825776}, {"filename": "/instruments/choir.pat", "start": 3825776, "end": 3898045}, {"filename": "/instruments/church.pat", "start": 3898045, "end": 4125756}, {"filename": "/instruments/claps.pat", "start": 4125756, "end": 4137475}, {"filename": "/instruments/clarinet.pat", "start": 4137475, "end": 4188778}, {"filename": "/instruments/clave.pat", "start": 4188778, "end": 4193813}, {"filename": "/instruments/clavinet.pat", "start": 4193813, "end": 4197926}, {"filename": "/instruments/cleangtr.pat", "start": 4197926, "end": 4742749}, {"filename": "/instruments/concrtna.pat", "start": 4742749, "end": 4760730}, {"filename": "/instruments/congahi1.pat", "start": 4760730, "end": 4769483}, {"filename": "/instruments/congahi2.pat", "start": 4769483, "end": 4781612}, {"filename": "/instruments/congalo.pat", "start": 4781612, "end": 4797701}, {"filename": "/instruments/contraba.pat", "start": 4797701, "end": 4807424}, {"filename": "/instruments/cowbell.pat", "start": 4807424, "end": 4828423}, {"filename": "/instruments/crystal.pat", "start": 4828423, "end": 4979892}, {"filename": "/instruments/cuica1.pat", "start": 4979892, "end": 4998887}, {"filename": "/instruments/cuica2.pat", "start": 4998887, "end": 5024904}, {"filename": "/instruments/cymbell.pat", "start": 5024904, "end": 5059719}, {"filename": "/instruments/cymchina.pat", "start": 5059719, "end": 5167208}, {"filename": "/instruments/cymcrsh1.pat", "start": 5167208, "end": 5230561}, {"filename": "/instruments/cymcrsh2.pat", "start": 5230561, "end": 5292972}, {"filename": "/instruments/cymride1.pat", "start": 5292972, "end": 5365535}, {"filename": "/instruments/cymride2.pat", "start": 5365535, "end": 5437896}, {"filename": "/instruments/cymsplsh.pat", "start": 5437896, "end": 5520243}, {"filename": "/instruments/distgtr.pat", "start": 5520243, "end": 5819416}, {"filename": "/instruments/doo.pat", "start": 5819416, "end": 6001359}, {"filename": "/instruments/echovox.pat", "start": 6001359, "end": 6026048}, {"filename": "/instruments/englhorn.pat", "start": 6026048, "end": 6050723}, {"filename": "/instruments/epiano1.pat", "start": 6050723, "end": 6359414}, {"filename": "/instruments/epiano2.pat", "start": 6359414, "end": 6435067}, {"filename": "/instruments/fiddle.pat", "start": 6435067, "end": 6447376}, {"filename": "/instruments/flute.pat", "start": 6447376, "end": 6459759}, {"filename": "/instruments/fngrbass.pat", "start": 6459759, "end": 6583592}, {"filename": "/instruments/frenchrn.pat", "start": 6583592, "end": 6700349}, {"filename": "/instruments/freshair.pat", "start": 6700349, "end": 6758656}, {"filename": "/instruments/fretless.pat", "start": 6758656, "end": 6921703}, {"filename": "/instruments/fx-blow.pat", "start": 6921703, "end": 6979396}, {"filename": "/instruments/fx-fret.pat", "start": 6979396, "end": 6992521}, {"filename": "/instruments/ghostie.pat", "start": 6992521, "end": 7055822}, {"filename": "/instruments/glocken.pat", "start": 7055822, "end": 7077773}, {"filename": "/instruments/gtrharm.pat", "start": 7077773, "end": 7087946}, {"filename": "/instruments/guiro1.pat", "start": 7087946, "end": 7092719}, {"filename": "/instruments/guiro2.pat", "start": 7092719, "end": 7119156}, {"filename": "/instruments/halopad.pat", "start": 7119156, "end": 7187671}, {"filename": "/instruments/hammond.pat", "start": 7187671, "end": 7790032}, {"filename": "/instruments/harmonca.pat", "start": 7790032, "end": 7805333}, {"filename": "/instruments/harp.pat", "start": 7805333, "end": 9026898}, {"filename": "/instruments/helicptr.pat", "start": 9026898, "end": 9092519}, {"filename": "/instruments/highq.pat", "start": 9092519, "end": 9096464}, {"filename": "/instruments/hihatcl.pat", "start": 9096464, "end": 9111045}, {"filename": "/instruments/hihatop.pat", "start": 9111045, "end": 9151646}, {"filename": "/instruments/hihatpd.pat", "start": 9151646, "end": 9160567}, {"filename": "/instruments/hitbrass.pat", "start": 9160567, "end": 9387144}, {"filename": "/instruments/honky.pat", "start": 9387144, "end": 9519049}, {"filename": "/instruments/hrpschrd.pat", "start": 9519049, "end": 9658798}, {"filename": "/instruments/jazzgtr.pat", "start": 9658798, "end": 9714721}, {"filename": "/instruments/jingles.pat", "start": 9714721, "end": 9748940}, {"filename": "/instruments/jungle.pat", "start": 9748940, "end": 9776479}, {"filename": "/instruments/kalimba.pat", "start": 9776479, "end": 9781218}, {"filename": "/instruments/kick1.pat", "start": 9781218, "end": 9790629}, {"filename": "/instruments/kick2.pat", "start": 9790629, "end": 9801006}, {"filename": "/instruments/koto.pat", "start": 9801006, "end": 9843085}, {"filename": "/instruments/lead5th.pat", "start": 9843085, "end": 9856318}, {"filename": "/instruments/maracas.pat", "start": 9856318, "end": 9866763}, {"filename": "/instruments/marcato.pat", "start": 9866763, "end": 9989644}, {"filename": "/instruments/marimba.pat", "start": 9989644, "end": 9994091}, {"filename": "/instruments/metalpad.pat", "start": 9994091, "end": 10054996}, {"filename": "/instruments/metbell.pat", "start": 10054996, "end": 10103479}, {"filename": "/instruments/metclick.pat", "start": 10103479, "end": 10110592}, {"filename": "/instruments/musicbox.pat", "start": 10110592, "end": 10141539}, {"filename": "/instruments/mutegtr.pat", "start": 10141539, "end": 10175074}, {"filename": "/instruments/mutetrum.pat", "start": 10175074, "end": 10469833}, {"filename": "/instruments/newage.pat", "start": 10469833, "end": 10558826}, {"filename": "/instruments/nyguitar.pat", "start": 10558826, "end": 11427305}, {"filename": "/instruments/oboe.pat", "start": 11427305, "end": 11485856}, {"filename": "/instruments/ocarina.pat", "start": 11485856, "end": 11489097}, {"filename": "/instruments/odguitar.pat", "start": 11489097, "end": 11545658}, {"filename": "/instruments/orchhit.pat", "start": 11545658, "end": 11574409}, {"filename": "/instruments/percorg.pat", "start": 11574409, "end": 11589844}, {"filename": "/instruments/piccolo.pat", "start": 11589844, "end": 11637485}, {"filename": "/instruments/pickbass.pat", "start": 11637485, "end": 11706638}, {"filename": "/instruments/pistol.pat", "start": 11706638, "end": 11768903}, {"filename": "/instruments/pizzcato.pat", "start": 11768903, "end": 12091556}, {"filename": "/instruments/polysyn.pat", "start": 12091556, "end": 12152315}, {"filename": "/instruments/recorder.pat", "start": 12152315, "end": 12163628}, {"filename": "/instruments/reedorg.pat", "start": 12163628, "end": 12167099}, {"filename": "/instruments/revcym.pat", "start": 12167099, "end": 12194490}, {"filename": "/instruments/rockorg.pat", "start": 12194490, "end": 12255377}, {"filename": "/instruments/santur.pat", "start": 12255377, "end": 12769042}, {"filename": "/instruments/sawwave.pat", "start": 12769042, "end": 12823527}, {"filename": "/instruments/scratch1.pat", "start": 12823527, "end": 12832618}, {"filename": "/instruments/scratch2.pat", "start": 12832618, "end": 12837501}, {"filename": "/instruments/seashore.pat", "start": 12837501, "end": 12899908}, {"filename": "/instruments/shakazul.pat", "start": 12899908, "end": 12962497}, {"filename": "/instruments/shaker.pat", "start": 12962497, "end": 12977016}, {"filename": "/instruments/shamisen.pat", "start": 12977016, "end": 13003683}, {"filename": "/instruments/shannai.pat", "start": 13003683, "end": 13023834}, {"filename": "/instruments/sitar.pat", "start": 13023834, "end": 13060813}, {"filename": "/instruments/slap.pat", "start": 13060813, "end": 13072844}, {"filename": "/instruments/slapbas1.pat", "start": 13072844, "end": 13110943}, {"filename": "/instruments/slapbas2.pat", "start": 13110943, "end": 13197622}, {"filename": "/instruments/slowstr.pat", "start": 13197622, "end": 13234273}, {"filename": "/instruments/snap.pat", "start": 13234273, "end": 13242834}, {"filename": "/instruments/snare1.pat", "start": 13242834, "end": 13275421}, {"filename": "/instruments/snare2.pat", "start": 13275421, "end": 13309582}, {"filename": "/instruments/snarerol.pat", "start": 13309582, "end": 13403747}, {"filename": "/instruments/soundtrk.pat", "start": 13403747, "end": 13443838}, {"filename": "/instruments/sprnosax.pat", "start": 13443838, "end": 13487585}, {"filename": "/instruments/sqrclick.pat", "start": 13487585, "end": 13488180}, {"filename": "/instruments/sqrwave.pat", "start": 13488180, "end": 13529889}, {"filename": "/instruments/startrak.pat", "start": 13529889, "end": 13584974}, {"filename": "/instruments/steeldrm.pat", "start": 13584974, "end": 13637125}, {"filename": "/instruments/stickrim.pat", "start": 13637125, "end": 13655326}, {"filename": "/instruments/sticks.pat", "start": 13655326, "end": 13673519}, {"filename": "/instruments/surdo1.pat", "start": 13673519, "end": 13693046}, {"filename": "/instruments/surdo2.pat", "start": 13693046, "end": 13712573}, {"filename": "/instruments/sweeper.pat", "start": 13712573, "end": 13775318}, {"filename": "/instruments/synbass1.pat", "start": 13775318, "end": 13848055}, {"filename": "/instruments/synbass2.pat", "start": 13848055, "end": 13918112}, {"filename": "/instruments/synbras1.pat", "start": 13918112, "end": 13979847}, {"filename": "/instruments/synbras2.pat", "start": 13979847, "end": 14040488}, {"filename": "/instruments/synpiano.pat", "start": 14040488, "end": 14301941}, {"filename": "/instruments/synstr1.pat", "start": 14301941, "end": 14364704}, {"filename": "/instruments/synstr2.pat", "start": 14364704, "end": 14397869}, {"filename": "/instruments/syntom.pat", "start": 14397869, "end": 14459200}, {"filename": "/instruments/taiko.pat", "start": 14459200, "end": 14496871}, {"filename": "/instruments/tamborin.pat", "start": 14496871, "end": 14529074}, {"filename": "/instruments/telephon.pat", "start": 14529074, "end": 14538231}, {"filename": "/instruments/tenorsax.pat", "start": 14538231, "end": 14584422}, {"filename": "/instruments/timbaleh.pat", "start": 14584422, "end": 14602595}, {"filename": "/instruments/timbalel.pat", "start": 14602595, "end": 14622606}, {"filename": "/instruments/timpani.pat", "start": 14622606, "end": 14661937}, {"filename": "/instruments/tomhi1.pat", "start": 14661937, "end": 14675404}, {"filename": "/instruments/tomhi2.pat", "start": 14675404, "end": 14688859}, {"filename": "/instruments/tomlo1.pat", "start": 14688859, "end": 14702314}, {"filename": "/instruments/tomlo2.pat", "start": 14702314, "end": 14721841}, {"filename": "/instruments/tommid1.pat", "start": 14721841, "end": 14735296}, {"filename": "/instruments/tommid2.pat", "start": 14735296, "end": 14748751}, {"filename": "/instruments/toms.pat", "start": 14748751, "end": 14762218}, {"filename": "/instruments/tremstr.pat", "start": 14762218, "end": 14915993}, {"filename": "/instruments/triangl1.pat", "start": 14915993, "end": 14924432}, {"filename": "/instruments/triangl2.pat", "start": 14924432, "end": 14968611}, {"filename": "/instruments/trombone.pat", "start": 14968611, "end": 14993948}, {"filename": "/instruments/trump2.pat", "start": 14993948, "end": 15779503}, {"filename": "/instruments/trumpet.pat", "start": 15779503, "end": 16324388}, {"filename": "/instruments/tuba.pat", "start": 16324388, "end": 16473539}, {"filename": "/instruments/tubebell.pat", "start": 16473539, "end": 16492144}, {"filename": "/instruments/unicorn.pat", "start": 16492144, "end": 16557439}, {"filename": "/instruments/vibes.pat", "start": 16557439, "end": 16579036}, {"filename": "/instruments/vibslap.pat", "start": 16579036, "end": 16624007}, {"filename": "/instruments/viola.pat", "start": 16624007, "end": 16679934}, {"filename": "/instruments/violin.pat", "start": 16679934, "end": 17322863}, {"filename": "/instruments/voices.pat", "start": 17322863, "end": 17353150}, {"filename": "/instruments/voxlead.pat", "start": 17353150, "end": 17398503}, {"filename": "/instruments/warmpad.pat", "start": 17398503, "end": 17434994}, {"filename": "/instruments/whistle.pat", "start": 17434994, "end": 17447047}, {"filename": "/instruments/whistle1.pat", "start": 17447047, "end": 17455496}, {"filename": "/instruments/whistle2.pat", "start": 17455496, "end": 17489207}, {"filename": "/instruments/woodblk.pat", "start": 17489207, "end": 17500228}, {"filename": "/instruments/woodblk1.pat", "start": 17500228, "end": 17508259}, {"filename": "/instruments/woodblk2.pat", "start": 17508259, "end": 17519280}, {"filename": "/instruments/woodflut.pat", "start": 17519280, "end": 17648855}, {"filename": "/instruments/xylophon.pat", "start": 17648855, "end": 17726874}, {"filename": "/timidity.cfg", "start": 17726874, "end": 17732668}], "remote_package_size": 17732668, "package_uuid": "sha256-2f969fa764a3fefe1ce8fc5718ba0eb5b304dff59eae290c27899d31757b2cf0"});

  })();
