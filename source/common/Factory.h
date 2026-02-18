//
// Created by Davis Polito on 1/22/26.
//

#ifndef BITKLAVIER0_FACTORY_H
#define BITKLAVIER0_FACTORY_H

#include <any>
#include <functional>
#include <map>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <iostream>
#include <string>

namespace factory_detail
{
    // Map "ReturnPtr" to the underlying Base type:
    template <class Base, class ReturnPtr>
    struct base_of_ptr
    {
        using type = Base;
    };

    template <class Base>
    struct base_of_ptr<Base, Base*>
    {
        using type = Base;
    };

    template <class Base>
    struct base_of_ptr<Base, std::unique_ptr<Base>>
    {
        using type = Base;
    };

    template <class Base>
    struct base_of_ptr<Base, std::shared_ptr<Base>>
    {
        using type = Base;
    };

    template <class Base, class ReturnPtr>
    using base_of_ptr_t = typename base_of_ptr<Base, ReturnPtr>::type;

    // How to construct ReturnPtr<T> given ctor args
    template <class ReturnPtr>
    struct maker;

    template <>
    struct maker<void*>; // not used

    template <class Base>
    struct maker<std::unique_ptr<Base>>
    {
        template <class T, class... CtorArgs>
        static std::unique_ptr<Base> make(CtorArgs&&... a)
        {
            static_assert(std::is_base_of_v<Base, T>);
            return std::make_unique<T>(std::forward<CtorArgs>(a)...);
        }
    };

    template <class Base>
    struct maker<std::shared_ptr<Base>>
    {
        template <class T, class... CtorArgs>
        static std::shared_ptr<Base> make(CtorArgs&&... a)
        {
            static_assert(std::is_base_of_v<Base, T>);
            return std::make_shared<T>(std::forward<CtorArgs>(a)...);
        }
    };

    template <class Base>
    struct maker<Base*>
    {
        template <class T, class... CtorArgs>
        static Base* make(CtorArgs&&... a)
        {
            static_assert(std::is_base_of_v<Base, T>);
            return new T(std::forward<CtorArgs>(a)...); // caller owns
        }
    };
} // namespace factory_detail


// ReturnPtr can be: std::unique_ptr<Base>, std::shared_ptr<Base>, or Base*
template <class Base, class ReturnPtr = std::unique_ptr<Base>>
class Factory
{
    static_assert(
        std::is_same_v<ReturnPtr, std::unique_ptr<Base>> ||
        std::is_same_v<ReturnPtr, std::shared_ptr<Base>> ||
        std::is_same_v<ReturnPtr, Base*>,
        "Factory ReturnPtr must be std::unique_ptr<Base>, std::shared_ptr<Base>, or Base*"
    );

public:
    using Ptr = ReturnPtr;
    using CreateFunction = std::function<Ptr(std::any)>;

    template <typename T, typename... Args>
    void registerType(const std::string& typeName)
    {
        creators[typeName] = [](std::any args) -> Ptr {
            try {
                // IMPORTANT: requires caller to store exactly std::tuple<Args...> inside std::any
                auto tupleArgs = std::any_cast<std::tuple<Args...>>(args);

                return std::apply(
                    [](auto&&... unpackedArgs) -> Ptr {
                        return factory_detail::maker<Ptr>::template make<T>(
                            std::forward<decltype(unpackedArgs)>(unpackedArgs)...
                        );
                    },
                    tupleArgs
                );
            }
            catch (const std::bad_any_cast& e) {
                std::cerr << "std::bad_any_cast: " << e.what() << " (expected tuple)" << std::endl;
                return Ptr{}; // nullptr / empty ptr
            }
        };
    }

    Ptr create(const std::string& typeName, std::any args) const
    {
        auto it = creators.find(typeName);
        if (it != creators.end())
            return it->second(std::move(args));
        return Ptr{};
    }

    bool contains(const std::string& str) const
    {
        return creators.find(str) != creators.end();
    }

private:
    std::map<std::string, CreateFunction> creators;
};

#endif // BITKLAVIER0_FACTORY_H
