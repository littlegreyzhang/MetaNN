#pragma once
#include <MetaNN/layers/facilities/policies.h>
#include <MetaNN/layers/facilities/traits.h>
#include <MetaNN/policies/policy_operations.h>
#include <MetaNN/policies/policy_selector.h>

namespace MetaNN
{
    template <typename TInputs, typename TPolicies>
    class InterpolateLayer
    {
        static_assert(IsPolicyContainer<TPolicies>);
        using CurLayerPolicy = PlainPolicy<TPolicies>;

    public:
        static constexpr bool IsFeedbackOutput = PolicySelect<GradPolicy, CurLayerPolicy>::IsFeedbackOutput;
        static constexpr bool IsUpdate = false;
        
        using InputPortSet = LayerPortSet<struct InterpolateLayerWeight1,
                                          struct InterpolateLayerWeight2,
                                          struct InterpolateLayerLambda>;
        using OutputPortSet = LayerPortSet<struct LayerOutput>;
        using InputMap = typename std::conditional_t<std::is_same_v<TInputs, NullParameter>,
                                                     EmptyLayerIOMap_<InputPortSet>,
                                                     Identity_<TInputs>>::type;
        static_assert(CheckInputMapAvailable_<InputMap, InputPortSet>::value);
        
    private:
        using TInterpolateLayerWeight1FP = typename InputMap::template Find<InterpolateLayerWeight1>;
        using TInterpolateLayerWeight2FP = typename InputMap::template Find<InterpolateLayerWeight2>;
        using TInterpolateLayerLambdaFP  = typename InputMap::template Find<InterpolateLayerLambda>;

    public:
        InterpolateLayer(std::string name)
            : m_name(std::move(name))
        {}
        
        template <typename TIn>
        auto FeedForward(TIn&& p_in)
        {
            auto input1 = LayerTraits::PickItemFromCont<InputMap, InterpolateLayerWeight1>(std::forward<TIn>(p_in));
            auto input2 = LayerTraits::PickItemFromCont<InputMap, InterpolateLayerWeight2>(std::forward<TIn>(p_in));
            auto lambda = LayerTraits::PickItemFromCont<InputMap, InterpolateLayerLambda>(std::forward<TIn>(p_in));
            auto proShape = LayerTraits::ShapePromote(input1, input2, lambda);
            auto res = Interpolate(Duplicate(input1, proShape),
                                   Duplicate(input2, proShape),
                                   Duplicate(lambda, proShape));

            if constexpr (IsFeedbackOutput)
            {
                m_weight1Shape.PushDataShape(input1);
                m_weight2Shape.PushDataShape(input2);
                m_lambdaShape.PushDataShape(lambda);

                m_input1Stack.push(std::move(input1));
                m_input2Stack.push(std::move(input2));
                m_lambdaStack.push(std::move(lambda));
            }

            return LayerOutputCont<InterpolateLayer>().template Set<LayerOutput>(std::move(res));
        }

        template <typename TGrad>
        auto FeedBackward(TGrad&& p_grad)
        {
            if constexpr (!IsFeedbackOutput || RemConstRef<TGrad>::template IsValueEmpty<LayerOutput>)
            {
                if constexpr (IsFeedbackOutput)
                {
                    LayerTraits::PopoutFromStack(m_lambdaStack, m_input1Stack, m_input2Stack,
                                                 m_weight1Shape, m_weight2Shape, m_lambdaShape);
                }
                return LayerInputCont<InterpolateLayer>();
            }
            else
            {
                if ((m_input1Stack.empty()) || (m_input2Stack.empty()) || (m_lambdaStack.empty()))
                {
                    throw std::runtime_error("Cannot do FeedBackward for InterpolateLayer");
                }
                auto grad = std::forward<TGrad>(p_grad).template Get<LayerOutput>();
                
                auto curLambda = m_lambdaStack.top();
                auto curInput1 = m_input1Stack.top();
                auto curInput2 = m_input2Stack.top();

                auto res2 = grad * Duplicate(1 - curLambda, grad.Shape());
                auto res1 = grad * Duplicate(curLambda, grad.Shape());
                auto resLambda = grad * (Duplicate(curInput1, grad.Shape()) - Duplicate(curInput2, grad.Shape()));
                
                auto out1 = Collapse(std::move(res1), curInput1.Shape());
                auto out2 = Collapse(std::move(res2), curInput2.Shape());
                auto outLambda = Collapse(std::move(resLambda), curLambda.Shape());
                
                m_weight1Shape.CheckDataShape(out1);
                m_weight2Shape.CheckDataShape(out2);
                m_lambdaShape.CheckDataShape(outLambda);

                LayerTraits::PopoutFromStack(m_lambdaStack, m_input1Stack, m_input2Stack,
                                             m_weight1Shape, m_weight2Shape, m_lambdaShape);
                return LayerInputCont<InterpolateLayer>()
                    .template Set<InterpolateLayerWeight1>(std::move(out1))
                    .template Set<InterpolateLayerWeight2>(std::move(out2))
                    .template Set<InterpolateLayerLambda>(std::move(outLambda));
            }
        }

        void NeutralInvariant()
        {
            if constexpr(IsFeedbackOutput)
            {
                LayerTraits::CheckStackEmpty(m_lambdaStack, m_input1Stack, m_input2Stack,
                                             m_weight1Shape, m_weight2Shape, m_lambdaShape);
            }
        }
    private:
        std::string m_name;
        LayerTraits::LayerInternalBuf<TInterpolateLayerWeight1FP, IsFeedbackOutput> m_input1Stack;
        LayerTraits::LayerInternalBuf<TInterpolateLayerWeight2FP, IsFeedbackOutput> m_input2Stack;
        LayerTraits::LayerInternalBuf<TInterpolateLayerLambdaFP,  IsFeedbackOutput> m_lambdaStack;

        LayerTraits::ShapeChecker<TInterpolateLayerWeight1FP,  IsFeedbackOutput> m_weight1Shape;
        LayerTraits::ShapeChecker<TInterpolateLayerWeight2FP,  IsFeedbackOutput> m_weight2Shape;
        LayerTraits::ShapeChecker<TInterpolateLayerLambdaFP,   IsFeedbackOutput> m_lambdaShape;
    };
}