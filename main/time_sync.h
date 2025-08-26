#pragma once
void time_sync_start(void);
int64_t time_sync_now_epoch(void); // seconds since epoch (0 if unsynced)
