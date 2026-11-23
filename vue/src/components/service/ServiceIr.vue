<script setup lang="ts">
import type { IListItem } from '@/types';

import { computed, ref, watch } from 'vue';

import { KEY, CMD } from '@/generated/protocol';
import { useConnection } from '@/composables/useConnection';
import { useLocale } from '@/composables/useLocale';

import { VButton, VCard, VCheckbox, VSelect, VTextField } from 'vuesp-components/components';

const { $t } = useLocale();

interface IHistoryLine {
  id: number;
  protocolName: string;
  address: number;
  code: number;
  value: number;
  time: string;
}

const { main, onSend } = useConnection((send) => {
  send(KEY.IR, { command: CMD.GET });
});

const ir = computed(() => main.value.ir);

const enabled = ref(true);
const dirty = ref(false);
const sendProtocol = ref(1);
const sendAddress = ref('0');
const sendCode = ref('0');
const sendRepeats = ref('0');
const sendError = ref('');
const history = ref<IHistoryLine[]>([]);
let historyId = 0;
let lastCount = 0;

const protocolItems: IListItem<number>[] = [
  { name: $t('ir.nec'), value: 1 },
  { name: $t('ir.raw'), value: 2 },
];

const selectName = <T,>(items: IListItem<T>[], value: T) => items.find((i) => i.value === value)?.name ?? String(value);

watch(
  () => main.value.ir,
  (value) => {
    if (!dirty.value) enabled.value = Boolean(value.enabled);
    if (value.count < lastCount) lastCount = 0;
    if (value.count > lastCount) {
      history.value.unshift({
        id: ++historyId,
        protocolName: value.protocolName || '—',
        address: value.address,
        code: value.code,
        value: value.value,
        time: new Date().toLocaleTimeString(),
      });
      history.value = history.value.slice(0, 20);
      lastCount = value.count;
    }
  },
  { immediate: true },
);

const hex = (value: number, digits = 4) => `0x${value.toString(16).toUpperCase().padStart(digits, '0')}`;

const onEnabled = (value: boolean) => {
  enabled.value = value;
  dirty.value = true;
};

const onApply = () => {
  onSend(KEY.IR, { ...ir.value, command: CMD.SET, enabled: enabled.value ? 1 : 0 });
  dirty.value = false;
};

const onDefaults = () => {
  enabled.value = true;
  sendProtocol.value = 1;
  sendAddress.value = '0';
  sendCode.value = '0';
  sendRepeats.value = '0';
  dirty.value = true;
  onSend(KEY.IR, { ...ir.value, command: CMD.RESTORE });
  dirty.value = false;
};

const onProtocol = ({ value }: IListItem<number>) => {
  sendProtocol.value = value;
  dirty.value = true;
};

const toInt = (value: string) => {
  const parsed = Number.parseInt(value, 0);
  return Number.isFinite(parsed) ? Math.max(0, parsed) : 0;
};

const onUseLast = () => {
  sendProtocol.value = 1;
  sendAddress.value = String(ir.value.address);
  sendCode.value = String(ir.value.code);
  sendRepeats.value = '0';
  dirty.value = true;
};

const onSendRaw = () => {
  sendProtocol.value = 2;
  onSendCode();
};

const onSendCode = () => {
  sendError.value = '';
  if (sendProtocol.value === 2 && !ir.value.rawLen) {
    sendError.value = $t('ir.empty');
    return;
  }

  onSend(KEY.IR, {
    ...ir.value,
    command: CMD.WRITE,
    sendProtocol: sendProtocol.value,
    sendAddress: toInt(sendAddress.value),
    sendCode: toInt(sendCode.value),
    sendRepeats: Math.min(20, toInt(sendRepeats.value)),
    raw: ir.value.raw,
    rawLen: ir.value.rawLen,
  });
};
</script>

<template>
  <div class="flex flex-col gap-4">
    <VCard variant="glass" padding="lg" class="relative z-20 overflow-visible">
      <div class="flex flex-wrap items-start justify-between gap-4">
        <div>
          <h3 class="text-base font-semibold">{{ $t('ir.settings') }}</h3>
          <p class="mt-1 text-xs text-gray-dark-25 dark:text-gray-light-10">{{ $t('ir.subtitle') }}</p>
        </div>
        <div class="flex flex-wrap items-center gap-3 text-sm">
          <span
            class="rounded-full px-3 py-1 text-xs font-semibold"
            :class="ir.status === 1 ? 'bg-emerald-500/10 text-emerald-500' : 'bg-amber-500/10 text-amber-500'"
          >
            {{ ir.status === 1 ? $t('ir.receiver') : $t('ir.receiverOff') }}
          </span>
          <span class="font-mono text-xs text-gray-dark-25 dark:text-gray-light-10">{{ $t('ir.count') }} {{ ir.count }} · {{ $t('ir.sent') }} {{ ir.sent }}</span>
        </div>
      </div>

      <div class="mt-4 flex flex-wrap items-center gap-4">
        <VCheckbox :model-value="enabled" @update:model-value="onEnabled">{{ $t('ir.enabled') }}</VCheckbox>
        <VButton color="blue" @click="onApply">{{ $t('ir.apply') }}</VButton>
        <VButton variant="outline" color="gray" @click="onDefaults">{{ $t('ir.defaults') }}</VButton>
        <span v-if="dirty" class="text-xs text-amber-600 dark:text-amber-400">{{ $t('ir.unsaved') }}</span>
      </div>
    </VCard>

    <div class="grid grid-cols-1 gap-4 xl:grid-cols-2">
      <VCard variant="glass" padding="lg" class="relative z-10">
        <div class="flex flex-wrap items-center justify-between gap-4">
          <h3 class="text-base font-semibold">{{ $t('ir.lastCode') }}</h3>
          <span class="font-mono text-xs text-gray-dark-25 dark:text-gray-light-10">{{ $t('ir.count') }} {{ ir.count }}</span>
        </div>

        <div class="mt-4 grid grid-cols-2 gap-3 text-sm">
          <div>
            <div class="text-xs text-gray-dark-25 dark:text-gray-light-10">{{ $t('ir.protocol') }}</div>
            <div class="mt-1 font-medium">{{ ir.protocolName || $t('ir.noCode') }}</div>
          </div>
          <div>
            <div class="text-xs text-gray-dark-25 dark:text-gray-light-10">{{ $t('ir.bits') }}</div>
            <div class="mt-1 font-mono font-medium">{{ ir.bits }}</div>
          </div>
          <div>
            <div class="text-xs text-gray-dark-25 dark:text-gray-light-10">{{ $t('ir.address') }}</div>
            <div class="mt-1 font-mono font-medium">{{ hex(ir.address) }}</div>
          </div>
          <div>
            <div class="text-xs text-gray-dark-25 dark:text-gray-light-10">{{ $t('ir.code') }}</div>
            <div class="mt-1 font-mono font-medium">{{ hex(ir.code) }}</div>
          </div>
          <div>
            <div class="text-xs text-gray-dark-25 dark:text-gray-light-10">{{ $t('ir.value') }}</div>
            <div class="mt-1 font-mono font-medium">{{ hex(ir.value, 8) }}</div>
          </div>
          <div>
            <div class="text-xs text-gray-dark-25 dark:text-gray-light-10">{{ $t('ir.repeats') }}</div>
            <div class="mt-1 font-mono font-medium">{{ ir.repeats }}</div>
          </div>
        </div>

        <div class="mt-4">
          <div class="text-xs text-gray-dark-25 dark:text-gray-light-10">{{ $t('ir.rawLen') }}: {{ ir.rawLen }}</div>
          <p class="mt-1 break-all font-mono text-xs text-gray-dark-25 dark:text-gray-light-10">
            {{ ir.rawLen ? ir.raw.slice(0, 40).join(' ') : $t('ir.empty') }}
          </p>
        </div>

        <div v-if="history.length" class="mt-4 border-t border-white/10 pt-3">
          <div class="text-xs text-gray-dark-25 dark:text-gray-light-10">{{ $t('ir.lastCode') }}</div>
          <div v-for="line in history" :key="line.id" class="mt-2 flex flex-wrap items-center gap-2 font-mono text-xs">
            <span class="text-gray-dark-25 dark:text-gray-light-10">{{ line.time }}</span>
            <span>{{ line.protocolName }}</span>
            <span>{{ hex(line.address) }}</span>
            <span>{{ hex(line.code) }}</span>
          </div>
        </div>
      </VCard>

      <VCard variant="glass" padding="lg" class="relative z-0">
        <h3 class="text-base font-semibold">{{ $t('ir.transmitter') }}</h3>

        <div class="mt-4 flex flex-col gap-4">
          <VSelect :model-value="selectName(protocolItems, sendProtocol)" :items="protocolItems" :label="$t('ir.protocol')" @change="onProtocol" />

          <div class="grid grid-cols-1 gap-3 sm:grid-cols-3">
            <VTextField :model-value="sendAddress" :label="$t('ir.address')" @update:model-value="(v) => (sendAddress = v)" />
            <VTextField :model-value="sendCode" :label="$t('ir.code')" @update:model-value="(v) => (sendCode = v)" />
            <VTextField :model-value="sendRepeats" :label="$t('ir.repeats')" @update:model-value="(v) => (sendRepeats = v)" />
          </div>

          <div class="flex flex-wrap items-center gap-3">
            <VButton color="blue" @click="onSendCode">{{ $t('ir.send') }}</VButton>
            <VButton variant="outline" color="gray" @click="onUseLast">{{ $t('ir.sendLast') }}</VButton>
            <VButton v-if="ir.rawLen" variant="ghost" color="gray" @click="onSendRaw">{{ $t('ir.sendRaw') }}</VButton>
          </div>
        </div>

        <p v-if="sendError" class="mt-3 text-xs text-red-500">{{ sendError }}</p>
        <p class="mt-3 text-xs text-gray-dark-25 dark:text-gray-light-10">{{ $t('ir.hint') }}</p>
      </VCard>
    </div>
  </div>
</template>
