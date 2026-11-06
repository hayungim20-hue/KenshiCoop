# KenshiCoop 修复版 / Fix Release — 0.58.0-local2

基于 [Savas07/KenshiCoop](https://github.com/Savas07/KenshiCoop) v0.58.0
（commit `4b60185d3a2651463a13412c132fe87c52d5bdda`）修改，2026 年 11 月。
Modified from Savas07/KenshiCoop v0.58.0 (commit `4b60185`), November 2026.

网络协议 **v73**：两位玩家必须安装同一个版本，旧版本会被拒绝连接。
Wire protocol **v73**: both players must run this exact build; older builds are refused.

---

## 中文

### 崩溃
- 联机中从地上捡过物品后，读档或退出游戏会崩溃 —— 已修复（被删除的物品残留在背包里）。
- 地面物品追踪、武器掉落检测、背包读取会访问已被引擎释放的物品而崩溃 —— 已加存活检查。
- 物品转移被拒后回滚会每帧无限重试（实测 26 分钟 14 万次后崩溃）—— 改为每秒一次、最多 15 次。

### 不同步 / "平行世界"
- 两位玩家之间互相挪人（换队）后，被挪的角色会变成没人控制的副本，两边各打各的 —— 已修复，双向都能正确交接控制权。
- 换队后对方队伍里凭空多出一个 0 级新角色 —— 已修复。
- 招募来的人换到对方队伍不再复制出第二个人。
- 读档时同一个 Steam 账号只占一个玩家位（之前朋友可能被分到错误编号，操控不了自己的队伍）。
- 版本不一致时直接拒绝连接，不会再"连上了但各跑各的"。
- 区域回到视野时被隐藏的 NPC 会恢复；区域交接后不再把整片 NPC 误判为不存在。
- 区块分管默认关闭，所有 NPC 统一由房主管理（更稳定；可用 `KENSHICOOP_CELL_AUTH=1` 打开）。
- 门不再在两边每秒来回开关。
- 对方队员的副本被本地 AI 放上床后会被拉回。

### 战斗
- NPC 受到的伤害在所有方向都会同步（房主打朋友管理的 NPC、朋友之间互打的 NPC）。
- 暗杀打晕会同步。
- 目标倒地后 NPC 不再对着空气摆战斗姿势。

### 物品与交易
- 在交易窗口里直接替对方角色买东西 / 卖对方角色的东西，现在会真正到账（之前钱扣了东西没了）。
- 朋友打开商人时，商品和商人的钱会从房主那边完整同步；商店打开时是空的会自动进货。
- 朋友那边的空商店不会再把房主那边商人的钱清零。
- 卖掉的东西不再掉回地上、凭空复制。
- 尸体和昏迷者的物品统一由房主管理，不会被两人重复搜刮；搜刮昏迷 NPC 会正确同步。
- 对方缺背包时会补建背包并补齐里面的物品（之前会引发一连串复制）。

### 建造与房产
- 建造放置和进度在对方还没加载时会保留重试，不再丢失；帮忙建造的进度会同步且不会倒退。
- 同步生成的建筑归玩家阵营所有（之前会变成城镇阵营，提示"贸易距离太远"）。
- 买房后两边都会清掉房内的非玩家家具。

### 使用体验
- 每次启动会把上一次的联机日志和崩溃包自动归档到游戏目录的 `KenshiCoop-logs\`（保留最近 10 份），出问题后重启不会再丢证据。
- F2 面板快捷键关闭，请用标题界面的 MULTIPLAYER 按钮。

### 已知限制
- 存档里原本就有的建筑，拆除进度不同步 —— 请由房主来拆。
- 个别剧情 NPC（Dialogue.mod 模板）可能在朋友那边不显示。
- 联机存档名只能用英文字母和数字；读档请先回主菜单再读。
- 与 KenshiQOL 等其他插件的兼容性未验证。
- 实验性质，请先备份存档。

---

## English

### Crashes
- Loading or quitting after a ground item had been picked up during co-op could crash (a destroyed item stayed listed in a bag) — fixed.
- Ground-item tracking, weapon-drop detection and inventory reads could touch items the engine had already freed — liveness checks added.
- A refused item-transfer rollback retried every frame forever (149,729 times in 26 min, then a crash) — now 1 Hz, at most 15 tries.

### Desync / "parallel world"
- Moving a character between the two players' squads left an uncontrolled copy on one side (the two games diverged) — control now hands over correctly in both directions.
- A brand-new level-0 character appeared in the other squad after a squad move — fixed.
- A recruit moved into the other player's squad no longer duplicates.
- On load, one Steam account maps to one player slot (a friend could end up unable to control their own squad).
- Mismatched builds are refused instead of connecting and silently diverging.
- Hidden NPCs are restored when their area streams back; an area handover no longer mass-hides NPCs.
- Per-cell NPC authority is off by default; the host manages every NPC (more stable; `KENSHICOOP_CELL_AUTH=1` re-enables it).
- Doors no longer flip open/closed between the two games.
- A peer's squad copy put to bed by the local AI is pulled back.

### Combat
- NPC damage syncs in every direction (host hitting join-managed NPCs, join-to-join).
- Assassination knockouts sync.
- NPCs drop their combat stance once the target is down.

### Items & trading
- Buying for / selling from the other player's character in the trade window now actually arrives (money was spent, items vanished).
- When the friend opens a trader, the host's full stock and trader money are mirrored; a shop that opens empty restocks itself.
- The friend's empty shop no longer zeroes the host trader's money.
- Sold items no longer reappear on the ground.
- Corpse / unconscious loot is host-managed, so it cannot be looted twice; looting a knocked-out NPC syncs.
- A missing backpack is rebuilt together with its contents (this used to start a duplication chain).

### Building & property
- Building placement/progress is kept and retried until the other side has loaded it; helper progress syncs and never goes backwards.
- Synced buildings belong to the player faction (no more "trade distance too far").
- Buying a house clears the non-player furniture on both games.

### Quality of life
- On every launch the previous co-op log and crash dump are archived to `<Kenshi>\KenshiCoop-logs\` (last 10 kept).
- F2 panel hotkey disabled; use the MULTIPLAYER button on the title screen.

### Known limitations
- Dismantling a building that was already in the save does not sync its progress — let the host dismantle.
- Some story NPCs (Dialogue.mod templates) may not appear for the friend.
- Co-op save names must be ASCII letters/digits; load from the main menu.
- Compatibility with other plugins (e.g. KenshiQOL) is untested.
- Experimental — back up your saves.
