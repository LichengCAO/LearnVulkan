#ifndef COMPONENT_H
#define COMPONENT_H

#include <memory>
#include <string>
#include <functional>
#include <vector>
#include <algorithm>
// https://stackoverflow.com/questions/44105058/implementing-component-system-from-unity-in-c

#define COMPONENT_DECLARATION                                        \
public:                                                              \
    static const std::size_t Type;                                   \
    virtual bool IsClassType(const std::size_t _type) const override;\

#define COMPONENT_DEFINITION(_parentClass, _thisClass)               \
const std::size_t _thisClass::Type = std::hash<std::string>()(#_thisClass);\
bool _thisClass::IsClassType(const std::size_t _type) const          \
{                                                                    \
    if (_thisClass::Type == _type) return true;                      \
    return _parentClass::IsClassType(_type);                         \
}                                                                    \

class Component
{
public:
	static const std::size_t Type;
	virtual bool IsClassType(const std::size_t _type) const { return _type == Component::Type; }
};

class ComponentManager
{
private:
	std::vector<std::unique_ptr<Component>> m_components;

public:
	// Add component to the object
	template<class ComponentType, typename... Args>
	void AddComponent(Args&&... _params);

	// Get the first component of the type, if not find return nullptr
	template<class ComponentType>
	ComponentType* GetComponent();

	// Get the first component of the type, if not find return nullptr
	template<class ComponentType>
	const ComponentType* GetComponent() const;

	// Push components of the type into the output vector
	template<class ComponentType>
	void GetComponents(std::vector<ComponentType*>& _outComponents);

	// Push components of the type into the output vector
	template<class ComponentType>
	void GetComponents(std::vector<const ComponentType*>& _outComponents) const;

	// Remove the first component of the type, return whether we really remove an element of this type
	template<class ComponentType>
	bool RemoveComponent();

	// Remove all components of the type, return number of components removed.
	template<class ComponentType>
	std::size_t RemoveComponents();
};

template<class ComponentType, typename... Args>
void ComponentManager::AddComponent(Args&&... _params)
{
	m_components.emplace_back(std::make_unique<ComponentType>(std::forward<Args>(_params)...));
}

template<class ComponentType>
ComponentType* ComponentManager::GetComponent()
{
	if (m_components.empty()) return nullptr;
	auto& itr = find_if(
		m_components.begin(),
		m_components.end(),
		[classType = ComponentType::Type](auto& component)
		{ 
			return component->IsClassType(classType); 
		});

	if (itr == m_components.end()) return nullptr;
	return (*itr).get();
}

template<class ComponentType>
const ComponentType* ComponentManager::GetComponent() const
{
	if (m_components.empty()) return nullptr;
	const auto& itr = find_if(
		m_components.begin(),
		m_components.end(),
		[classType = ComponentType::Type](const auto& component)
	{
		return component->IsClassType(classType);
	});

	if (itr == m_components.end()) return nullptr;
	return reinterpret_cast<ComponentType*>((*itr).get());
}

template<class ComponentType>
void ComponentManager::GetComponents(std::vector<ComponentType*>& _outComponents)
{
	for (auto&& uptr : m_components)
	{
		if (uptr->IsClassType(ComponentType::Type)) _outComponents.emplace_back(uptr.get());
	}
}

template<class ComponentType>
void ComponentManager::GetComponents(std::vector<const ComponentType*>& _outComponents) const
{
	for (auto&& uptr : m_components)
	{
		if (uptr->IsClassType(ComponentType::Type)) _outComponents.emplace_back(reinterpret_cast<ComponentType*>(uptr.get()));
	}
}

template<class ComponentType>
bool ComponentManager::RemoveComponent()
{
	if (m_components.empty()) return false;
	auto& itr = find_if(
		m_components.begin(),
		m_components.end(),
		[classType = ComponentType::Type](auto& component)
		{
			return component->IsClassType(classType);
		});
	if (itr == m_components.end()) return false;
	m_components.erase(itr);
	return true;
}

template<class ComponentType>
std::size_t ComponentManager::RemoveComponents()
{
	if (m_components.empty()) return 0;
	std::size_t nRemoveCount = 0;
	bool bFound = false;

	do 
	{
		auto& itr = find_if(
			m_components.begin(),
			m_components.end(),
			[classType = ComponentType::Type](auto& component)
			{
				return component->IsClassType(classType);
			});
		bFound = (itr != m_components.end());
		if (bFound)
		{
			m_components.erase(itr);
			++nRemoveCount;
		}
	} while (bFound);

	return nRemoveCount;
}

#endif // !COMPONENT_H

#ifdef COMPONENT_IMPLEMENTATION
#undef COMPONENT_IMPLEMENTATION
const std::size_t Component::Type = std::hash<std::string>()("Component");
#endif