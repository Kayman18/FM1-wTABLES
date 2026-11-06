#pragma once
/* Original integer-dB table: -40 is mute, -39..24 are 10^(dB/20). */
float pw_gain_db(int db);
