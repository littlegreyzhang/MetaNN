#pragma once

#include <MetaNN/data/facilities/traits.h>
#include <MetaNN/evaluate/eval_buffer.h>
#include <MetaNN/evaluate/eval_plan.h>
#include <cstring>
#include <type_traits>

namespace MetaNN
{
    namespace NSZeroTensor
    {
        template <typename TElem, typename TDevice, size_t uDim>
        class EvalItem : public BaseEvalItem<TDevice>
        {
        public:
            using CategoryTag = CategoryTags::Tensor<uDim>;
            using ElementType = TElem;
            using DeviceType = TDevice;

            EvalItem(EvalHandle<PrincipalDataType<CategoryTag, ElementType, DeviceType>> resBuf,
                     Shape<uDim> p_shape)
                : BaseEvalItem<TDevice>(std::type_index(typeid(EvalItem)),
                                        {}, resBuf.DataPtr())
                , m_resHandle(std::move(resBuf))
                , m_shape(std::move(p_shape))
            {
            }
        
            EvalHandle<PrincipalDataType<CategoryTag, ElementType, DeviceType>> m_resHandle;
            const Shape<uDim> m_shape;
        };

        template <typename TElem, typename TDevice, size_t uDim>
        class EvalGroup : public TrivalEvalGroup<EvalItem<TElem, TDevice, uDim>>
        {
            using EvalItemType = EvalItem<TElem, TDevice, uDim>;
        protected:
            virtual void EvalInternalLogic(EvalItemType& evalItem) final override
            {
                using CategoryTag = CategoryTags::Tensor<uDim>;
                PrincipalDataType<CategoryTag, TElem, TDevice> res(evalItem.m_shape);
                auto lowLayer = LowerAccess(res);
                auto mem = lowLayer.MutableRawMemory();
        
                static_assert(std::is_same_v<TDevice, DeviceTags::CPU>, 
                              "Memset not support for other device tag.");
                memset(mem, 0, sizeof(TElem) * evalItem.m_shape.Count());
                evalItem.m_resHandle.SetData(std::move(res));
            }
        };
    }

    template <typename TElem, typename TDevice, size_t uDim>
    class ZeroTensor
    {
    public:
        using CategoryTag = CategoryTags::Tensor<uDim>;
        using ElementType = TElem;
        using DeviceType = TDevice;

    public:
        template <typename...TShapeParams>
        explicit ZeroTensor(TShapeParams&&... shapeParams)
            : m_shape(std::forward<TShapeParams>(shapeParams)...)
        {}
    
        const auto& Shape() const noexcept
        {
            return m_shape;
        }

        bool operator== (const ZeroTensor& val) const
        {
            return (m_shape == val.m_shape);
        }

        auto EvalRegister() const
        {
            using TEvalItem = NSZeroTensor::EvalItem<ElementType, DeviceType, uDim>;
            using TEvalGroup = NSZeroTensor::EvalGroup<ElementType, DeviceType, uDim>;
            using TItemDispatcher = TrivalEvalItemDispatcher<TEvalGroup>;

            if (!m_evalBuf.IsEvaluated())
            {
                auto evalHandle = m_evalBuf.Handle();
                if (!EvalPlan<DeviceType>::Inst().IsAlreayRegisted(evalHandle.DataPtr()))
                {
                    EvalPlan<DeviceType>::Inst().template Register<TItemDispatcher>(
                        std::make_unique<TEvalItem>(std::move(evalHandle), m_shape));
                }
            }
            return m_evalBuf.ConstHandle();
        }

    private:
        MetaNN::Shape<uDim> m_shape;
        EvalBuffer<PrincipalDataType<CategoryTag, ElementType, DeviceType>> m_evalBuf;
    };
}