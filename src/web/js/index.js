import { Elm } from "../elm/Main";
import { SRB2 } from "./SRB2";
import "../style.css";

const app = Elm.Main.init({ node: document.getElementById("main") });
const srb2 = new SRB2(app);

const supportsKeyboardLock = ('keyboard' in navigator) && ('lock' in navigator.keyboard);
if (supportsKeyboardLock) {
  document.addEventListener('fullscreenchange', async () => {
    if (document.fullscreenElement) {
      await navigator.keyboard.lock(['Escape']);        
      return; 
    }
    navigator.keyboard.unlock();
  });
}

app.ports.startGame.subscribe(srb2.init);
app.ports.listWads.subscribe(() => srb2.Command_ListWADS_f());
app.ports.requestFullScreen.subscribe(() => srb2.requestFullscreen());
app.ports.addFile.subscribe((message) => {
  // We handle this in JavaScript because serializing 60MBs of
  // binary data in base64 is pretty bad!
  const input = document.createElement("input");
  input.type = "file";
  input.accept = ".wad ,.pk3, ,.soc, ,.lua"
  input.addEventListener("change", (event) => {
    const file = event.target.files[0];
    srb2.addFile(file);
  });

  input.click();
});
