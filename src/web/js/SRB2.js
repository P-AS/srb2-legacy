export class SRB2 {
  constructor(app) {
    this.canvas = document.getElementById("canvas");
    this.app = app;

    this.Module = {
      preRun: [],
      postRun: [],
      print: (function () {
        return function (t) {
          const text = SRB2.parseEmsText(t);
          console.log(text);
          app.ports.gameOutput.send(text);
        };
      })(),
      stdin: {},
      printErr: (t) => {
        const text = SRB2.parseEmsText(t);
        console.log(text);
        this.app.ports.gameOutput.send(text);
      },
      canvas: this.canvas,
      setStatus: (text) => {
        this.app.ports.statusMessage.send(text);
      },
      totalDependencies: 1,
      monitorRunDependencies: function (left) {
        // console.log("LEFT", left);
        // this.totalDependencies = Math.max(this.totalDependencies, left);
        // Module.setStatus(
        //   left
        //     ? "Preparing... (" +
        //         (this.totalDependencies - left) +
        //         "/" +
        //         this.totalDependencies +
        //         ")"
        //     : "All downloads complete."
        // );
      },
    };

    window.Module = this.Module;
  } 
  
  static parseEmsText(text) {
    if (arguments.length > 1) {
      return Array.prototype.slice.call(arguments).join(" ");
    } else {
      return text;
    }
  }

  setupMethods = () => {
    this.P_AddWadFile = this.Module.cwrap(
      "P_AddWadFile",
      "boolean",
      ["string"],
      ["string"]
    );
    this.Command_ListWADS_f = this.Module.cwrap("Command_ListWADS_f", "void");
  };

  addFile = async (file) => {
    const filename = file.name;
    const fileStream = FS.open(filename, "w");

    for await (const chunk of file.stream()) {
      FS.write(fileStream, chunk, 0, chunk.length);
    }

    this.P_AddWadFile(filename);
    FS.close(fileStream)
  };

  requestFullscreen = () => {
    this.canvas.requestFullscreen();
  };

  init = () => {
    const SRB2Script = document.createElement("script");
    SRB2Script.setAttribute("src", "srb2legacy.js");

    SRB2Script.setAttribute("type", "text/javascript");
    SRB2Script.setAttribute("async", true);
    SRB2Script.addEventListener("load", () => {
      this.setupMethods();
    });

    document.body.appendChild(SRB2Script);
  };
}
