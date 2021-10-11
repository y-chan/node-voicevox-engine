import * as fs from 'fs'

import Engine from '@/index'

const engine = new Engine('core.dll', false)
const speaker_id = 0
const query = engine.audio_query('こんにちは、四国めたんです。', speaker_id)
console.log(JSON.stringify(query, null, 2))

const wave_buf = engine.synthesis(query, speaker_id)

fs.writeFileSync('hello.wav', wave_buf)
