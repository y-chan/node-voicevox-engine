import fastify, { FastifySchema } from 'fastify'
import fastifyCors from 'fastify-cors'
import fastifySwagger from 'fastify-swagger'

import Engine, { AccentPhrase, AudioQuery } from '@/index'
import packageJson from '@/package.json'

const server = fastify({
  logger: true,
})
void server.register(fastifySwagger, {
  routePrefix: '/docs',
  openapi: {
    info: {
      title: 'Node VOICEVOX Engine',
      description: 'VOICEVOXの音声合成エンジンです。',
      version: packageJson.version,
    },
  },
  exposeRoute: true,
})
void server.register(fastifyCors, {
  origin: '*',
  credentials: true,
  methods: '*',
  allowedHeaders: '*',
})

const PORT = process.env.PORT || 50021
const engine = new Engine('core.dll', false)
console.log('loading core was succeed')

interface AudioQueryApiQuery {
  text: string
  speaker: number
}

interface AccentPhrasesApiQuery {
  text: string
  speaker: number
  is_kana?: boolean
}

interface MoraApiQuery {
  accent_phrases: AccentPhrase[]
  speaker: number
}

interface SynthesisApiQuery {
  audio_query: AudioQuery
  speaker: number
}

interface QueryString<Q> {
  Querystring: Q
}

const MoraSchema = {
  type: 'object',
  required: ['text', 'vowel', 'vowel_length', 'pitch'],
  properties: {
    text: { type: 'string' },
    consonant: { type: 'string' },
    consonant_length: { type: 'number' },
    vowel: { type: 'string' },
    vowel_length: { type: 'number' },
    pitch: { type: 'number' },
  },
}

const AccentPhrasesSchema = {
  type: 'array',
  items: {
    type: 'object',
    required: ['moras', 'accent'],
    properties: {
      moras: {
        type: 'array',
        items: MoraSchema,
      },
      accent: { type: 'number' },
      pause_mora: MoraSchema,
    },
  },
}

const AudioQuerySchema = {
  types: 'object',
  required: [
    'accent_phrases',
    'speedScale',
    'pitchScale',
    'intonationScale',
    'volumeScale',
    'prePhonemeLength',
    'postPhonemeLength',
    'outputSamplingRate',
    'outputStereo',
    'kana',
  ],
  properties: {
    accent_phrases: AccentPhrasesSchema,
    speedScale: { type: 'number' },
    pitchScale: { type: 'number' },
    intonationScale: { type: 'number' },
    volumeScale: { type: 'number' },
    prePhonemeLength: { type: 'number' },
    postPhonemeLength: { type: 'number' },
    outputSamplingRate: { type: 'number' },
    outputStereo: { type: 'boolean' },
    kana: { type: 'string' },
  },
}

const AudioQueryApiSchema: FastifySchema = {
  querystring: {
    type: 'object',
    required: ['text', 'speaker'],
    properties: {
      text: { type: 'string' },
      speaker: { type: 'number' },
    },
  },
  response: {
    200: AudioQuerySchema,
  },
}

const AccentPhrasesApiSchema: FastifySchema = {
  querystring: {
    type: 'object',
    required: ['text', 'speaker'],
    properties: {
      text: { type: 'string' },
      speaker: { type: 'number' },
      is_kana: { type: 'boolean' },
    },
  },
  response: {
    200: AccentPhrasesSchema,
  },
}

const MoraApiSchema: FastifySchema = {
  querystring: {
    type: 'object',
    required: ['accent_phrases', 'speaker'],
    properties: {
      accent_phrases: AccentPhrasesSchema,
      speaker: { type: 'number' },
    },
  },
  response: {
    200: AccentPhrasesSchema,
  },
}

const SynthesisApiSchema: FastifySchema = {
  querystring: {
    type: 'object',
    required: ['audio_query', 'speaker'],
    properties: {
      audio_query: AudioQuerySchema,
      speaker: { type: 'number' },
    },
  },
}

server.post<QueryString<AudioQueryApiQuery>>(
  '/audio_query',
  {
    schema: AudioQueryApiSchema,
  },
  async (request, reply) => {
    try {
      const result = engine.audio_query(
        request.query.text,
        request.query.speaker
      )
      void reply.type('application/json').code(200)
      return result
    } catch (e) {
      void reply.type('application/json').code(400)
      return { error: String(e) }
    }
  }
)

server.post<QueryString<AccentPhrasesApiQuery>>(
  '/accent_phrases',
  { schema: AccentPhrasesApiSchema },
  async (request, reply) => {
    try {
      const result = engine.accent_phrases(
        request.query.text,
        request.query.speaker,
        request.query.is_kana
      )
      void reply.type('application/json').code(200)
      return result
    } catch (e) {
      void reply.type('application/json').code(400)
      return { error: String(e) }
    }
  }
)

server.post<QueryString<MoraApiQuery>>(
  '/mora_data',
  {
    schema: MoraApiSchema,
  },
  async (request, reply) => {
    try {
      const result = engine.mora_data(
        request.query.accent_phrases,
        request.query.speaker
      )
      void reply.type('application/json').code(200)
      return result
    } catch (e) {
      void reply.type('application/json').code(400)
      return { error: String(e) }
    }
  }
)

server.post<QueryString<MoraApiQuery>>(
  '/mora_length',
  {
    schema: MoraApiSchema,
  },
  async (request, reply) => {
    try {
      const result = engine.mora_length(
        request.query.accent_phrases,
        request.query.speaker
      )
      void reply.type('application/json').code(200)
      return result
    } catch (e) {
      void reply.type('application/json').code(400)
      return { error: String(e) }
    }
  }
)

server.post<QueryString<MoraApiQuery>>(
  '/mora_pitch',
  {
    schema: MoraApiSchema,
  },
  async (request, reply) => {
    try {
      const result = engine.mora_pitch(
        request.query.accent_phrases,
        request.query.speaker
      )
      void reply.type('application/json').code(200)
      // void reply.type('audio/wav').code(200)
      return result
    } catch (e) {
      void reply.type('application/json').code(400)
      return { error: String(e) }
    }
  }
)

server.get('/version', async (request, reply) => {
  return packageJson.version
})

server.get('/speakers', async (request, reply) => {
  void reply.type('application/json').code(200)
  return engine.metas()
})

server.post<QueryString<SynthesisApiQuery>>(
  '/synthesis',
  {
    schema: SynthesisApiSchema,
  },
  async (request, reply) => {
    try {
      const result = engine.synthesis(
        request.query.audio_query,
        request.query.speaker
      )
      void reply.type('audio/wav').code(200)
      return result
    } catch (e) {
      void reply.type('application/json').code(400)
      return { error: String(e) }
    }
  }
)

server.listen(PORT, '0.0.0.0', (err, address) => {
  if (err) throw err
  server.log.info(`server listening on ${address}`)
})
