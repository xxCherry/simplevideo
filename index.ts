let binding: any;

try {
  binding = require("./dist/node-bindings.node");
} catch {
  binding = require("./dist/Release/node-bindings.node");
}

type TextureUpdateCb = (pixelsAddress: number) => void;
type NativeHandle = unknown;

export class Video {
  private videoExternal: NativeHandle;
  private isDisposed: boolean = false;

  path: string;
  width: number;
  height: number;

  constructor(path: string, width: number, height: number) {
    this.path = path;
    this.width = width;
    this.height = height;

    this.videoExternal = binding.videoCreate(path, width, height);

    this.start();
  }

  static enableLog(): void {
    binding.videoEnableLog();
  }

  static disableLog(): void {
    binding.videoDisableLog();
  }

  private start(): void {
    binding.videoStart(this.videoExternal);
  }

  play(): void {
    binding.videoPlay(this.videoExternal);
  }

  pause(): void {
    binding.videoPause(this.videoExternal);
  }

  isPlaying(): boolean {
    return binding.videoIsPlaying(this.videoExternal);
  }

  update(cb: TextureUpdateCb): void {
    binding.videoUpdate(this.videoExternal, cb);
  }

  seek(time: number): void {
    binding.videoSeek(this.videoExternal, time);
  }

  get time(): number {
    return binding.videoGetTime(this.videoExternal);
  }

  get duration(): number {
    return binding.videoGetDuration(this.videoExternal);
  }

  get speed(): number {
    return binding.videoGetSpeed(this.videoExternal);
  }

  set speed(value: number) {
    binding.videoSetSpeed(this.videoExternal, value);
  }

  get loop(): boolean {
    return binding.videoGetLoop(this.videoExternal);
  }

  set loop(value: boolean) {
    binding.videoSetLoop(this.videoExternal, value);
  }

  dispose() {
    if (this.isDisposed) {
      return;
    }

    binding.videoDestroy(this.videoExternal);

    this.isDisposed = true;
  }

  [Symbol.dispose]() {
    this.dispose();
  }
}
