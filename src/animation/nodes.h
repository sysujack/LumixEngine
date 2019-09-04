#pragma once

#include "engine/array.h"


namespace Lumix
{

struct Pose;

namespace Anim
{

struct GroupNode;

struct RuntimeContext {
	RuntimeContext(Controller& controller, IAllocator& allocator);

	void setInput(u32 input_idx, float value);
	void setInput(u32 input_idx, bool value);

	Controller& controller;
	Array<u8> inputs;
	Array<Animation*> animations;
	OutputMemoryStream data;
	
	u32 root_bone_hash = 0;
	float time_delta;
	class Model* model = nullptr;
	InputMemoryStream input_runtime;
};


struct Node {
	enum Type : u32 {
		ANIMATION,
		GROUP,
		BLEND1D
	};

	Node(GroupNode* parent, IAllocator& allocator) 
		: m_name("", allocator)
		, m_parent(parent)
	{}

	virtual ~Node() {}
	virtual Type type() const = 0;
	virtual void update(RuntimeContext& ctx, Ref<LocalRigidTransform> root_motion) const = 0;
	virtual void enter(RuntimeContext& ctx) const = 0;
	virtual void skip(RuntimeContext& ctx) const = 0;
	virtual void getPose(RuntimeContext& ctx, float weight, Ref<Pose> pose) const = 0;
	virtual void serialize(OutputMemoryStream& stream) const = 0 {
		stream.writeString(m_name.c_str());
	}

	virtual void deserialize(InputMemoryStream& stream, Controller& ctrl) = 0 {
		char tmp[64];
		stream.readString(tmp, sizeof(tmp));
		m_name = tmp;
	}

	static Node* create(GroupNode* parent, Type type, IAllocator& allocator);

	GroupNode* m_parent;
	String m_name;
};

struct AnimationNode final : Node {
	AnimationNode(GroupNode* parent, IAllocator& allocator);
	Type type() const override { return ANIMATION; }
	
	void update(RuntimeContext& ctx, Ref<LocalRigidTransform> root_motion) const override;
	void enter(RuntimeContext& ctx) const override;
	void skip(RuntimeContext& ctx) const override;
	void getPose(RuntimeContext& ctx, float weight, Ref<Pose> pose) const override;
	void serialize(OutputMemoryStream& stream) const override;
	void deserialize(InputMemoryStream& stream, Controller& ctrl) override;

	u32 m_slot;
};

struct Blend1DNode final : Node {
	Blend1DNode(GroupNode* parent, IAllocator& allocator);
	Type type() const override { return BLEND1D; }
	
	void update(RuntimeContext& ctx, Ref<LocalRigidTransform> root_motion) const override;
	void enter(RuntimeContext& ctx) const override;
	void skip(RuntimeContext& ctx) const override;
	void getPose(RuntimeContext& ctx, float weight, Ref<Pose> pose) const override;
	void serialize(OutputMemoryStream& stream) const override;
	void deserialize(InputMemoryStream& stream, Controller& ctrl) override;

	struct Child {
		float value = 0;
		u32 slot = 0;
	};

	Array<Child> m_children;
	u32 m_input_index = 0;
};

struct GroupNode final : Node {
	GroupNode(GroupNode* parent, IAllocator& allocator);
	Type type() const override { return GROUP; }

	void update(RuntimeContext& ctx, Ref<LocalRigidTransform> root_motion) const override;
	void enter(RuntimeContext& ctx) const override;
	void skip(RuntimeContext& ctx) const override;
	void getPose(RuntimeContext& ctx, float weight, Ref<Pose> pose) const override;
	void serialize(OutputMemoryStream& stream) const override;
	void deserialize(InputMemoryStream& stream, Controller& ctrl) override;

	struct RuntimeData {
		u32 from;
		u32 to;
		float t;
	};

	struct Child {
		Child(IAllocator& allocator) 
			: condition(allocator) 
			, condition_str("", allocator)
		{}

		Condition condition;
		String condition_str;
		Node* node; 
	};

	IAllocator& m_allocator;
	float m_blend_length = 0.3f;
	Array<Child> m_children;
};



} // ns anim
} // ns Lumix