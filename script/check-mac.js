const is_mac = process.platform === 'darwin'

if (is_mac) {
  throw Error('現在、Mac上でNode VOICEVOX Coreを使うことはできません。')
}
