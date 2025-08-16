# AI Tool Call 升级 - 精确角度控制

## 🚀 新功能概述

升级了AI自动驾驶的Tool Call功能，现在支持更精确的角度控制和多种移动模式，让机器人的移动更加智能和精确。

## 🔧 新增的电机控制功能

### 1. 精确角度转弯 (`turn_angle`)
- **功能**: 按指定角度转弯，支持-180到180度
- **参数**: `angle` (角度), `speed` (速度)
- **示例**: 向左转45度
```json
{
  "action": "turn_angle",
  "angle": -45,
  "speed": 60,
  "duration": 2.0
}
```

### 2. 原地转弯 (`pivot_turn`)
- **功能**: 原地旋转，转弯更快更精确
- **参数**: `angle` (角度), `speed` (速度)
- **示例**: 原地右转90度
```json
{
  "action": "pivot_turn", 
  "angle": 90,
  "speed": 70,
  "duration": 1.5
}
```

### 3. 差速驱动 (`differential_drive`)
- **功能**: 独立控制左右轮速度，实现曲线移动
- **参数**: `left_speed` (左轮速度), `right_speed` (右轮速度)
- **示例**: 左轮慢，右轮快，实现向左曲线移动
```json
{
  "action": "differential_drive",
  "left_speed": 40,
  "right_speed": 80,
  "duration": 3.0
}
```

## 🤖 AI的新能力

### 智能决策示例

**场景1: 精确避障**
```
AI思考: "前方有障碍物，我需要向右转约30度来避开它"
动作: turn_angle, angle: 30, speed: 50
```

**场景2: 狭窄空间转弯**
```
AI思考: "空间太狭窄，我需要原地转弯180度掉头"
动作: pivot_turn, angle: 180, speed: 60
```

**场景3: 曲线探索**
```
AI思考: "我想沿着墙壁缓慢移动，保持一定距离"
动作: differential_drive, left_speed: 30, right_speed: 50
```

## 📋 Tool Call参数详解

### 必需参数
- `reasoning`: AI的思考过程 (必须)
- `action`: 动作类型 (必须)
- `duration`: 持续时间 (必须)
- `speed`: 基础速度 (必须)

### 可选参数 (根据动作类型)
- `angle`: 转弯角度 (turn_angle, pivot_turn使用)
- `left_speed`: 左轮速度 (differential_drive使用)
- `right_speed`: 右轮速度 (differential_drive使用)

## 🔧 技术实现

### 电机驱动层
- `motor_turn_angle()`: 时间计算转弯
- `motor_pivot_turn()`: 左右轮反向旋转
- `motor_differential_drive()`: 独立控制左右轮

### AI服务层
- 扩展Tool Call参数解析
- 智能参数验证
- 详细执行日志
- 结果格式化存储

## 🎯 使用场景

1. **精确导航**: 需要特定角度转弯时
2. **狭窄空间**: 需要原地转弯时  
3. **曲线移动**: 需要沿着物体边缘移动时
4. **探索优化**: 需要更灵活的移动策略时

## 📊 性能优化

- 增加HTTP响应缓冲区到8192字节
- 优化JSON解析错误处理
- 改进Tool Call参数验证
- 增强调试信息输出

## 🔄 向后兼容

所有原有的基础动作(forward, backward, left, right, stop)保持完全兼容，只是增加了新的控制选项。

---

**升级完成时间**: 2024年
**版本**: v2.0
**状态**: ✅ 编译通过，ready for testing