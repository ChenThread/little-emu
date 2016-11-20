#define CPU_CLEAR_FLAGS(v) state->flag &= ~(v)
#define CPU_SET_Z(v) if ((v) == 0) { state->flag |= FLAG_Z; }
#define CPU_SET_N(v) if ((v) >= 0x80) { state->flag |= FLAG_N; }
#define CPU_SET_V(v) if (((v) < -0x80) || ((v) >= 0x80)) { state->flag |= FLAG_V; }
#define CPU_UPDATE_NZ(v) { CPU_CLEAR_FLAGS(FLAG_N | FLAG_V); CPU_SET_N((v)); CPU_SET_Z((v)); }
