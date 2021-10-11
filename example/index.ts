import Engine from '@/index'

const engine = new Engine('core.dll', false)
console.log(engine.audio_query('こんにちは、四国めたんです。', 0))
