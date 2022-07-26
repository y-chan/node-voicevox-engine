import * as fs from 'fs'

import Engine from '@/index'

const engine = new Engine('core.dll', false)
const speaker_id = 0
const word_uuid = engine.add_user_dict_word(
  'DeepLearning',
  'ディープラーニング',
  4
)
console.log(word_uuid)
console.log(engine.get_user_dict_words())
const query = engine.audio_query('DeepLearningをしています', speaker_id)
console.log(JSON.stringify(query, null, 2))
engine.delete_user_dict_word(word_uuid)

const wave_buf = engine.synthesis(query, speaker_id)

fs.writeFileSync('hello.wav', wave_buf)
