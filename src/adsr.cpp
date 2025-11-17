#include "adsr.h"
#include <math.h>
#include <SPI.h>

int last_adsr_output = -1;

ADSR::ADSR(
    int l_vertical_resolution, 
    float attack_alpha,
    float attack_decay_release
)
{
    // Initialise
    _vertical_resolution = l_vertical_resolution;

    _attach_alpha = attack_alpha;
    _attack_decay_release = attack_decay_release;

    _attack = DEFAULT_ADR_uS;
    _decay = DEFAULT_ADR_uS;
    _sustain = l_vertical_resolution * DEFAULT_SUSTAIN_LEVEL;                   
    _release = DEFAULT_ADR_uS;

    // Create look-up table for Attack
    for (int i = 0; i < LUT_SIZE; i++) {
        _attack_table[i] = i;
        _decay_release_table[i] = _vertical_resolution - 1 - i;
    }

      // Create look-up table for Decay and Release
    for (int i = 0; i < LUT_SIZE - 1; i++) {
        _attack_table[i+1] = (1.0 - _attach_alpha) * (_vertical_resolution - 1) + _attach_alpha * _attack_table[i];
        _decay_release_table[i+1] = _attack_decay_release * _decay_release_table[i];
    }

    // Normalize tables to min and max
    for (int i = 0; i < LUT_SIZE; i++) {
        _attack_table[i] = _map(
            _attack_table[i], 
            0, 
            _attack_table[LUT_SIZE - 1], 
            0, 
            _vertical_resolution - 1
        );
        
        _decay_release_table[i] = _map(
            _decay_release_table[i], 
            _decay_release_table[LUT_SIZE - 1], 
            _decay_release_table[0], 
            0, 
            _vertical_resolution - 1
        );
    }
}

void ADSR::set_reset_attack(bool l_reset_attack)
{
    _reset_attack = l_reset_attack;
}

void ADSR::set_attack(unsigned long l_attack)
{
    _attack = l_attack;
}

void ADSR::set_decay(unsigned long l_decay)
{
    _decay = l_decay;
}

void ADSR::set_sustain(int l_sustain)
{
    if (l_sustain < 0) {
        l_sustain = 0;
    }

    if (l_sustain >= _vertical_resolution) {
        l_sustain = _vertical_resolution - 1;
    }

    _sustain = l_sustain;
}

void ADSR::set_release(unsigned long l_release)
{
    _release = l_release;
}

void ADSR::note_on() {
    _t_note_on = _micros();                   // Set new timestamp for note_on

    // Set start value new Attack. If _reset_attack equals true, a new trigger starts with 0
    // otherwise start with the current output
    _attack_start = _reset_attack ? 0 : _adsr_output;
    
    _notes_pressed++;                               // increase number of pressed notes with one
}

void ADSR::note_off() {
    _notes_pressed--;
    if (_notes_pressed <= 0) {                      // if all notes are depressed - start release
        _t_note_off = _micros();                    // set timestamp for note off
        _release_start = _adsr_output;              // set start value for release
        _notes_pressed = 0;
    }
}

bool ADSR::is_on() {
    return _notes_pressed >= 1;
}

int ADSR::envelope()
{
    // Read time once to avoid tiny inconsistencies between multiple _micros() calls
    uint64_t now = _micros();
    unsigned long delta = 0;

    // if note is pressed
    if (_t_note_off < _t_note_on) {
        delta = (now > _t_note_on) ? (unsigned long)(now - _t_note_on) : 0;

        // Attack
        if (_attack == 0 || delta < _attack) {
            unsigned long attack_d = (_attack == 0) ? 0 : delta;
            float idx_f = (float)(LUT_SIZE - 1) * (float)attack_d / (float)max(1UL, _attack);
            int idx = (int)floorf(idx_f);
            float frac = idx_f - (float)idx;
            if (idx < 0) { idx = 0; frac = 0.0f; }
            if (idx >= LUT_SIZE - 1) { idx = LUT_SIZE - 2; frac = 1.0f; }

            float v0 = (float)_attack_table[idx];
            float v1 = (float)_attack_table[idx + 1];
            float table_val = (1.0f - frac) * v0 + frac * v1;

            float vmax = (float)(_vertical_resolution - 1);
            float out_f = ((table_val / vmax) * (vmax - (float)_attack_start)) + (float)_attack_start;
            _adsr_output = (int)roundf(out_f);

        // Decay
        } else if (delta < _attack + _decay) {
            unsigned long d2 = (now > _t_note_on + _attack) ? (unsigned long)(now - _t_note_on - _attack) : 0;
            float idx_f = (float)(LUT_SIZE - 1) * (float)d2 / (float)max(1UL, _decay);
            int idx = (int)floorf(idx_f);
            float frac = idx_f - (float)idx;
            if (idx < 0) { idx = 0; frac = 0.0f; }
            if (idx >= LUT_SIZE - 1) { idx = LUT_SIZE - 2; frac = 1.0f; }

            float v0 = (float)_decay_release_table[idx];
            float v1 = (float)_decay_release_table[idx + 1];
            float table_val = (1.0f - frac) * v0 + frac * v1;

            float vmax = (float)(_vertical_resolution - 1);
            float out_f = ((table_val / vmax) * (vmax - (float)_sustain)) + (float)_sustain;
            _adsr_output = (int)roundf(out_f);

        // Sustain is reached
        } else {
            _adsr_output = _sustain;
        }
    }

    // if note not pressed
    if (_t_note_off > _t_note_on) {
        delta = (now > _t_note_off) ? (unsigned long)(now - _t_note_off) : 0;

        // Release
        if (_release == 0 || delta < _release) {
            unsigned long rel_d = (_release == 0) ? 0 : delta;
            float idx_f = (float)(LUT_SIZE - 1) * (float)rel_d / (float)max(1UL, _release);
            int idx = (int)floorf(idx_f);
            float frac = idx_f - (float)idx;
            if (idx < 0) { idx = 0; frac = 0.0f; }
            if (idx >= LUT_SIZE - 1) { idx = LUT_SIZE - 2; frac = 1.0f; }

            float v0 = (float)_decay_release_table[idx];
            float v1 = (float)_decay_release_table[idx + 1];
            float table_val = (1.0f - frac) * v0 + frac * v1;

            float vmax = (float)(_vertical_resolution - 1);
            float out_f = (table_val / vmax) * (float)_release_start;
            _adsr_output = (int)roundf(out_f);

        // Release finished
        } else {
            _adsr_output = 0;
        }
    }

    // Debugging output
    if (last_adsr_output != _adsr_output) {
        last_adsr_output = _adsr_output;
     //   Serial.print(_adsr_output);
       // Serial.print(" - ");
      //  Serial.print(micros() - _t_note_off);
      //  Serial.println();
    }

    return _adsr_output;
}