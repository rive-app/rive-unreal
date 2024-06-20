#ifndef _RIVE_CORE_REGISTRY_HPP_
#define _RIVE_CORE_REGISTRY_HPP_
#include "rive/animation/advanceable_state.hpp"
#include "rive/animation/animation.hpp"
#include "rive/animation/animation_state.hpp"
#include "rive/animation/any_state.hpp"
#include "rive/animation/blend_animation.hpp"
#include "rive/animation/blend_animation_1d.hpp"
#include "rive/animation/blend_animation_direct.hpp"
#include "rive/animation/blend_state.hpp"
#include "rive/animation/blend_state_1d.hpp"
#include "rive/animation/blend_state_direct.hpp"
#include "rive/animation/blend_state_transition.hpp"
#include "rive/animation/cubic_ease_interpolator.hpp"
#include "rive/animation/cubic_interpolator.hpp"
#include "rive/animation/cubic_interpolator_component.hpp"
#include "rive/animation/cubic_value_interpolator.hpp"
#include "rive/animation/elastic_interpolator.hpp"
#include "rive/animation/entry_state.hpp"
#include "rive/animation/exit_state.hpp"
#include "rive/animation/interpolating_keyframe.hpp"
#include "rive/animation/keyed_object.hpp"
#include "rive/animation/keyed_property.hpp"
#include "rive/animation/keyframe.hpp"
#include "rive/animation/keyframe_bool.hpp"
#include "rive/animation/keyframe_callback.hpp"
#include "rive/animation/keyframe_color.hpp"
#include "rive/animation/keyframe_double.hpp"
#include "rive/animation/keyframe_id.hpp"
#include "rive/animation/keyframe_interpolator.hpp"
#include "rive/animation/keyframe_string.hpp"
#include "rive/animation/layer_state.hpp"
#include "rive/animation/linear_animation.hpp"
#include "rive/animation/listener_action.hpp"
#include "rive/animation/listener_align_target.hpp"
#include "rive/animation/listener_bool_change.hpp"
#include "rive/animation/listener_fire_event.hpp"
#include "rive/animation/listener_input_change.hpp"
#include "rive/animation/listener_number_change.hpp"
#include "rive/animation/listener_trigger_change.hpp"
#include "rive/animation/nested_bool.hpp"
#include "rive/animation/nested_input.hpp"
#include "rive/animation/nested_linear_animation.hpp"
#include "rive/animation/nested_number.hpp"
#include "rive/animation/nested_remap_animation.hpp"
#include "rive/animation/nested_simple_animation.hpp"
#include "rive/animation/nested_state_machine.hpp"
#include "rive/animation/nested_trigger.hpp"
#include "rive/animation/state_machine.hpp"
#include "rive/animation/state_machine_bool.hpp"
#include "rive/animation/state_machine_component.hpp"
#include "rive/animation/state_machine_fire_event.hpp"
#include "rive/animation/state_machine_input.hpp"
#include "rive/animation/state_machine_layer.hpp"
#include "rive/animation/state_machine_layer_component.hpp"
#include "rive/animation/state_machine_listener.hpp"
#include "rive/animation/state_machine_number.hpp"
#include "rive/animation/state_machine_trigger.hpp"
#include "rive/animation/state_transition.hpp"
#include "rive/animation/transition_bool_condition.hpp"
#include "rive/animation/transition_condition.hpp"
#include "rive/animation/transition_number_condition.hpp"
#include "rive/animation/transition_trigger_condition.hpp"
#include "rive/animation/transition_value_condition.hpp"
#include "rive/artboard.hpp"
#include "rive/assets/asset.hpp"
#include "rive/assets/audio_asset.hpp"
#include "rive/assets/drawable_asset.hpp"
#include "rive/assets/export_audio.hpp"
#include "rive/assets/file_asset.hpp"
#include "rive/assets/file_asset_contents.hpp"
#include "rive/assets/folder.hpp"
#include "rive/assets/font_asset.hpp"
#include "rive/assets/image_asset.hpp"
#include "rive/audio_event.hpp"
#include "rive/backboard.hpp"
#include "rive/bones/bone.hpp"
#include "rive/bones/cubic_weight.hpp"
#include "rive/bones/root_bone.hpp"
#include "rive/bones/skeletal_component.hpp"
#include "rive/bones/skin.hpp"
#include "rive/bones/tendon.hpp"
#include "rive/bones/weight.hpp"
#include "rive/component.hpp"
#include "rive/constraints/constraint.hpp"
#include "rive/constraints/distance_constraint.hpp"
#include "rive/constraints/follow_path_constraint.hpp"
#include "rive/constraints/ik_constraint.hpp"
#include "rive/constraints/rotation_constraint.hpp"
#include "rive/constraints/scale_constraint.hpp"
#include "rive/constraints/targeted_constraint.hpp"
#include "rive/constraints/transform_component_constraint.hpp"
#include "rive/constraints/transform_component_constraint_y.hpp"
#include "rive/constraints/transform_constraint.hpp"
#include "rive/constraints/transform_space_constraint.hpp"
#include "rive/constraints/translation_constraint.hpp"
#include "rive/container_component.hpp"
#include "rive/custom_property.hpp"
#include "rive/custom_property_boolean.hpp"
#include "rive/custom_property_number.hpp"
#include "rive/custom_property_string.hpp"
#include "rive/data_bind/data_bind.hpp"
#include "rive/data_bind/data_bind_context.hpp"
#include "rive/draw_rules.hpp"
#include "rive/draw_target.hpp"
#include "rive/drawable.hpp"
#include "rive/event.hpp"
#include "rive/joystick.hpp"
#include "rive/layout/layout_component_style.hpp"
#include "rive/layout_component.hpp"
#include "rive/layout_component_absolute.hpp"
#include "rive/nested_animation.hpp"
#include "rive/nested_artboard.hpp"
#include "rive/node.hpp"
#include "rive/open_url_event.hpp"
#include "rive/shapes/clipping_shape.hpp"
#include "rive/shapes/contour_mesh_vertex.hpp"
#include "rive/shapes/cubic_asymmetric_vertex.hpp"
#include "rive/shapes/cubic_detached_vertex.hpp"
#include "rive/shapes/cubic_mirrored_vertex.hpp"
#include "rive/shapes/cubic_vertex.hpp"
#include "rive/shapes/ellipse.hpp"
#include "rive/shapes/image.hpp"
#include "rive/shapes/mesh.hpp"
#include "rive/shapes/mesh_vertex.hpp"
#include "rive/shapes/paint/fill.hpp"
#include "rive/shapes/paint/gradient_stop.hpp"
#include "rive/shapes/paint/linear_gradient.hpp"
#include "rive/shapes/paint/radial_gradient.hpp"
#include "rive/shapes/paint/shape_paint.hpp"
#include "rive/shapes/paint/solid_color.hpp"
#include "rive/shapes/paint/stroke.hpp"
#include "rive/shapes/paint/trim_path.hpp"
#include "rive/shapes/parametric_path.hpp"
#include "rive/shapes/path.hpp"
#include "rive/shapes/path_vertex.hpp"
#include "rive/shapes/points_path.hpp"
#include "rive/shapes/polygon.hpp"
#include "rive/shapes/rectangle.hpp"
#include "rive/shapes/shape.hpp"
#include "rive/shapes/star.hpp"
#include "rive/shapes/straight_vertex.hpp"
#include "rive/shapes/triangle.hpp"
#include "rive/shapes/vertex.hpp"
#include "rive/solo.hpp"
#include "rive/text/text.hpp"
#include "rive/text/text_modifier.hpp"
#include "rive/text/text_modifier_group.hpp"
#include "rive/text/text_modifier_range.hpp"
#include "rive/text/text_shape_modifier.hpp"
#include "rive/text/text_style.hpp"
#include "rive/text/text_style_axis.hpp"
#include "rive/text/text_style_feature.hpp"
#include "rive/text/text_value_run.hpp"
#include "rive/text/text_variation_modifier.hpp"
#include "rive/transform_component.hpp"
#include "rive/viewmodel/data_enum.hpp"
#include "rive/viewmodel/data_enum_value.hpp"
#include "rive/viewmodel/viewmodel.hpp"
#include "rive/viewmodel/viewmodel_component.hpp"
#include "rive/viewmodel/viewmodel_instance.hpp"
#include "rive/viewmodel/viewmodel_instance_color.hpp"
#include "rive/viewmodel/viewmodel_instance_enum.hpp"
#include "rive/viewmodel/viewmodel_instance_list.hpp"
#include "rive/viewmodel/viewmodel_instance_list_item.hpp"
#include "rive/viewmodel/viewmodel_instance_number.hpp"
#include "rive/viewmodel/viewmodel_instance_string.hpp"
#include "rive/viewmodel/viewmodel_instance_value.hpp"
#include "rive/viewmodel/viewmodel_instance_viewmodel.hpp"
#include "rive/viewmodel/viewmodel_property.hpp"
#include "rive/viewmodel/viewmodel_property_color.hpp"
#include "rive/viewmodel/viewmodel_property_enum.hpp"
#include "rive/viewmodel/viewmodel_property_list.hpp"
#include "rive/viewmodel/viewmodel_property_number.hpp"
#include "rive/viewmodel/viewmodel_property_string.hpp"
#include "rive/viewmodel/viewmodel_property_viewmodel.hpp"
#include "rive/world_transform_component.hpp"
namespace rive
{
class CoreRegistry
{
public:
    static Core* makeCoreInstance(int typeKey)
    {
        switch (typeKey)
        {
            case ViewModelInstanceListItemBase::typeKey:
                return new ViewModelInstanceListItem();
            case ViewModelInstanceColorBase::typeKey:
                return new ViewModelInstanceColor();
            case ViewModelComponentBase::typeKey:
                return new ViewModelComponent();
            case ViewModelPropertyBase::typeKey:
                return new ViewModelProperty();
            case ViewModelPropertyNumberBase::typeKey:
                return new ViewModelPropertyNumber();
            case ViewModelInstanceEnumBase::typeKey:
                return new ViewModelInstanceEnum();
            case ViewModelInstanceStringBase::typeKey:
                return new ViewModelInstanceString();
            case ViewModelPropertyListBase::typeKey:
                return new ViewModelPropertyList();
            case ViewModelBase::typeKey:
                return new ViewModel();
            case ViewModelPropertyViewModelBase::typeKey:
                return new ViewModelPropertyViewModel();
            case ViewModelInstanceBase::typeKey:
                return new ViewModelInstance();
            case DataEnumBase::typeKey:
                return new DataEnum();
            case ViewModelPropertyEnumBase::typeKey:
                return new ViewModelPropertyEnum();
            case ViewModelPropertyColorBase::typeKey:
                return new ViewModelPropertyColor();
            case ViewModelInstanceListBase::typeKey:
                return new ViewModelInstanceList();
            case ViewModelInstanceNumberBase::typeKey:
                return new ViewModelInstanceNumber();
            case ViewModelPropertyStringBase::typeKey:
                return new ViewModelPropertyString();
            case ViewModelInstanceViewModelBase::typeKey:
                return new ViewModelInstanceViewModel();
            case DataEnumValueBase::typeKey:
                return new DataEnumValue();
            case DrawTargetBase::typeKey:
                return new DrawTarget();
            case CustomPropertyNumberBase::typeKey:
                return new CustomPropertyNumber();
            case DistanceConstraintBase::typeKey:
                return new DistanceConstraint();
            case IKConstraintBase::typeKey:
                return new IKConstraint();
            case FollowPathConstraintBase::typeKey:
                return new FollowPathConstraint();
            case TranslationConstraintBase::typeKey:
                return new TranslationConstraint();
            case TransformConstraintBase::typeKey:
                return new TransformConstraint();
            case ScaleConstraintBase::typeKey:
                return new ScaleConstraint();
            case RotationConstraintBase::typeKey:
                return new RotationConstraint();
            case NodeBase::typeKey:
                return new Node();
            case NestedArtboardBase::typeKey:
                return new NestedArtboard();
            case SoloBase::typeKey:
                return new Solo();
            case LayoutComponentStyleBase::typeKey:
                return new LayoutComponentStyle();
            case ListenerFireEventBase::typeKey:
                return new ListenerFireEvent();
            case NestedSimpleAnimationBase::typeKey:
                return new NestedSimpleAnimation();
            case AnimationStateBase::typeKey:
                return new AnimationState();
            case NestedTriggerBase::typeKey:
                return new NestedTrigger();
            case KeyedObjectBase::typeKey:
                return new KeyedObject();
            case AnimationBase::typeKey:
                return new Animation();
            case BlendAnimationDirectBase::typeKey:
                return new BlendAnimationDirect();
            case StateMachineNumberBase::typeKey:
                return new StateMachineNumber();
            case CubicValueInterpolatorBase::typeKey:
                return new CubicValueInterpolator();
            case TransitionTriggerConditionBase::typeKey:
                return new TransitionTriggerCondition();
            case KeyedPropertyBase::typeKey:
                return new KeyedProperty();
            case StateMachineListenerBase::typeKey:
                return new StateMachineListener();
            case KeyFrameIdBase::typeKey:
                return new KeyFrameId();
            case KeyFrameBoolBase::typeKey:
                return new KeyFrameBool();
            case ListenerBoolChangeBase::typeKey:
                return new ListenerBoolChange();
            case ListenerAlignTargetBase::typeKey:
                return new ListenerAlignTarget();
            case TransitionNumberConditionBase::typeKey:
                return new TransitionNumberCondition();
            case AnyStateBase::typeKey:
                return new AnyState();
            case CubicInterpolatorComponentBase::typeKey:
                return new CubicInterpolatorComponent();
            case StateMachineLayerBase::typeKey:
                return new StateMachineLayer();
            case KeyFrameStringBase::typeKey:
                return new KeyFrameString();
            case ListenerNumberChangeBase::typeKey:
                return new ListenerNumberChange();
            case CubicEaseInterpolatorBase::typeKey:
                return new CubicEaseInterpolator();
            case StateTransitionBase::typeKey:
                return new StateTransition();
            case NestedBoolBase::typeKey:
                return new NestedBool();
            case KeyFrameDoubleBase::typeKey:
                return new KeyFrameDouble();
            case KeyFrameColorBase::typeKey:
                return new KeyFrameColor();
            case StateMachineBase::typeKey:
                return new StateMachine();
            case StateMachineFireEventBase::typeKey:
                return new StateMachineFireEvent();
            case EntryStateBase::typeKey:
                return new EntryState();
            case LinearAnimationBase::typeKey:
                return new LinearAnimation();
            case StateMachineTriggerBase::typeKey:
                return new StateMachineTrigger();
            case ListenerTriggerChangeBase::typeKey:
                return new ListenerTriggerChange();
            case BlendStateDirectBase::typeKey:
                return new BlendStateDirect();
            case NestedStateMachineBase::typeKey:
                return new NestedStateMachine();
            case ElasticInterpolatorBase::typeKey:
                return new ElasticInterpolator();
            case ExitStateBase::typeKey:
                return new ExitState();
            case NestedNumberBase::typeKey:
                return new NestedNumber();
            case BlendState1DBase::typeKey:
                return new BlendState1D();
            case KeyFrameCallbackBase::typeKey:
                return new KeyFrameCallback();
            case NestedRemapAnimationBase::typeKey:
                return new NestedRemapAnimation();
            case TransitionBoolConditionBase::typeKey:
                return new TransitionBoolCondition();
            case BlendStateTransitionBase::typeKey:
                return new BlendStateTransition();
            case StateMachineBoolBase::typeKey:
                return new StateMachineBool();
            case BlendAnimation1DBase::typeKey:
                return new BlendAnimation1D();
            case LinearGradientBase::typeKey:
                return new LinearGradient();
            case RadialGradientBase::typeKey:
                return new RadialGradient();
            case StrokeBase::typeKey:
                return new Stroke();
            case SolidColorBase::typeKey:
                return new SolidColor();
            case GradientStopBase::typeKey:
                return new GradientStop();
            case TrimPathBase::typeKey:
                return new TrimPath();
            case FillBase::typeKey:
                return new Fill();
            case MeshVertexBase::typeKey:
                return new MeshVertex();
            case ShapeBase::typeKey:
                return new Shape();
            case StraightVertexBase::typeKey:
                return new StraightVertex();
            case CubicAsymmetricVertexBase::typeKey:
                return new CubicAsymmetricVertex();
            case MeshBase::typeKey:
                return new Mesh();
            case PointsPathBase::typeKey:
                return new PointsPath();
            case ContourMeshVertexBase::typeKey:
                return new ContourMeshVertex();
            case RectangleBase::typeKey:
                return new Rectangle();
            case CubicMirroredVertexBase::typeKey:
                return new CubicMirroredVertex();
            case TriangleBase::typeKey:
                return new Triangle();
            case EllipseBase::typeKey:
                return new Ellipse();
            case ClippingShapeBase::typeKey:
                return new ClippingShape();
            case PolygonBase::typeKey:
                return new Polygon();
            case StarBase::typeKey:
                return new Star();
            case ImageBase::typeKey:
                return new Image();
            case CubicDetachedVertexBase::typeKey:
                return new CubicDetachedVertex();
            case EventBase::typeKey:
                return new Event();
            case DrawRulesBase::typeKey:
                return new DrawRules();
            case CustomPropertyBooleanBase::typeKey:
                return new CustomPropertyBoolean();
            case LayoutComponentBase::typeKey:
                return new LayoutComponent();
            case ArtboardBase::typeKey:
                return new Artboard();
            case JoystickBase::typeKey:
                return new Joystick();
            case BackboardBase::typeKey:
                return new Backboard();
            case AbsoluteLayoutComponentBase::typeKey:
                return new AbsoluteLayoutComponent();
            case OpenUrlEventBase::typeKey:
                return new OpenUrlEvent();
            case DataBindBase::typeKey:
                return new DataBind();
            case DataBindContextBase::typeKey:
                return new DataBindContext();
            case WeightBase::typeKey:
                return new Weight();
            case BoneBase::typeKey:
                return new Bone();
            case RootBoneBase::typeKey:
                return new RootBone();
            case SkinBase::typeKey:
                return new Skin();
            case TendonBase::typeKey:
                return new Tendon();
            case CubicWeightBase::typeKey:
                return new CubicWeight();
            case TextModifierRangeBase::typeKey:
                return new TextModifierRange();
            case TextStyleFeatureBase::typeKey:
                return new TextStyleFeature();
            case TextVariationModifierBase::typeKey:
                return new TextVariationModifier();
            case TextModifierGroupBase::typeKey:
                return new TextModifierGroup();
            case TextStyleBase::typeKey:
                return new TextStyle();
            case TextStyleAxisBase::typeKey:
                return new TextStyleAxis();
            case TextBase::typeKey:
                return new Text();
            case TextValueRunBase::typeKey:
                return new TextValueRun();
            case CustomPropertyStringBase::typeKey:
                return new CustomPropertyString();
            case FolderBase::typeKey:
                return new Folder();
            case ImageAssetBase::typeKey:
                return new ImageAsset();
            case FontAssetBase::typeKey:
                return new FontAsset();
            case AudioAssetBase::typeKey:
                return new AudioAsset();
            case FileAssetContentsBase::typeKey:
                return new FileAssetContents();
            case AudioEventBase::typeKey:
                return new AudioEvent();
        }
        return nullptr;
    }
    static void setBool(Core* object, int propertyKey, bool value)
    {
        switch (propertyKey)
        {
            case ViewModelInstanceListItemBase::useLinkedArtboardPropertyKey:
                object->as<ViewModelInstanceListItemBase>()->useLinkedArtboard(value);
                break;
            case TransformComponentConstraintBase::offsetPropertyKey:
                object->as<TransformComponentConstraintBase>()->offset(value);
                break;
            case TransformComponentConstraintBase::doesCopyPropertyKey:
                object->as<TransformComponentConstraintBase>()->doesCopy(value);
                break;
            case TransformComponentConstraintBase::minPropertyKey:
                object->as<TransformComponentConstraintBase>()->min(value);
                break;
            case TransformComponentConstraintBase::maxPropertyKey:
                object->as<TransformComponentConstraintBase>()->max(value);
                break;
            case TransformComponentConstraintYBase::doesCopyYPropertyKey:
                object->as<TransformComponentConstraintYBase>()->doesCopyY(value);
                break;
            case TransformComponentConstraintYBase::minYPropertyKey:
                object->as<TransformComponentConstraintYBase>()->minY(value);
                break;
            case TransformComponentConstraintYBase::maxYPropertyKey:
                object->as<TransformComponentConstraintYBase>()->maxY(value);
                break;
            case IKConstraintBase::invertDirectionPropertyKey:
                object->as<IKConstraintBase>()->invertDirection(value);
                break;
            case FollowPathConstraintBase::orientPropertyKey:
                object->as<FollowPathConstraintBase>()->orient(value);
                break;
            case FollowPathConstraintBase::offsetPropertyKey:
                object->as<FollowPathConstraintBase>()->offset(value);
                break;
            case NestedSimpleAnimationBase::isPlayingPropertyKey:
                object->as<NestedSimpleAnimationBase>()->isPlaying(value);
                break;
            case KeyFrameBoolBase::valuePropertyKey:
                object->as<KeyFrameBoolBase>()->value(value);
                break;
            case ListenerAlignTargetBase::preserveOffsetPropertyKey:
                object->as<ListenerAlignTargetBase>()->preserveOffset(value);
                break;
            case NestedBoolBase::nestedValuePropertyKey:
                object->as<NestedBoolBase>()->nestedValue(value);
                break;
            case LinearAnimationBase::enableWorkAreaPropertyKey:
                object->as<LinearAnimationBase>()->enableWorkArea(value);
                break;
            case LinearAnimationBase::quantizePropertyKey:
                object->as<LinearAnimationBase>()->quantize(value);
                break;
            case StateMachineBoolBase::valuePropertyKey:
                object->as<StateMachineBoolBase>()->value(value);
                break;
            case ShapePaintBase::isVisiblePropertyKey:
                object->as<ShapePaintBase>()->isVisible(value);
                break;
            case StrokeBase::transformAffectsStrokePropertyKey:
                object->as<StrokeBase>()->transformAffectsStroke(value);
                break;
            case PointsPathBase::isClosedPropertyKey:
                object->as<PointsPathBase>()->isClosed(value);
                break;
            case RectangleBase::linkCornerRadiusPropertyKey:
                object->as<RectangleBase>()->linkCornerRadius(value);
                break;
            case ClippingShapeBase::isVisiblePropertyKey:
                object->as<ClippingShapeBase>()->isVisible(value);
                break;
            case CustomPropertyBooleanBase::propertyValuePropertyKey:
                object->as<CustomPropertyBooleanBase>()->propertyValue(value);
                break;
            case LayoutComponentBase::clipPropertyKey:
                object->as<LayoutComponentBase>()->clip(value);
                break;
            case TextModifierRangeBase::clampPropertyKey:
                object->as<TextModifierRangeBase>()->clamp(value);
                break;
        }
    }
    static void setUint(Core* object, int propertyKey, uint32_t value)
    {
        switch (propertyKey)
        {
            case ViewModelInstanceListItemBase::viewModelIdPropertyKey:
                object->as<ViewModelInstanceListItemBase>()->viewModelId(value);
                break;
            case ViewModelInstanceListItemBase::viewModelInstanceIdPropertyKey:
                object->as<ViewModelInstanceListItemBase>()->viewModelInstanceId(value);
                break;
            case ViewModelInstanceListItemBase::artboardIdPropertyKey:
                object->as<ViewModelInstanceListItemBase>()->artboardId(value);
                break;
            case ViewModelInstanceValueBase::viewModelPropertyIdPropertyKey:
                object->as<ViewModelInstanceValueBase>()->viewModelPropertyId(value);
                break;
            case ViewModelInstanceEnumBase::propertyValuePropertyKey:
                object->as<ViewModelInstanceEnumBase>()->propertyValue(value);
                break;
            case ViewModelBase::defaultInstanceIdPropertyKey:
                object->as<ViewModelBase>()->defaultInstanceId(value);
                break;
            case ViewModelPropertyViewModelBase::viewModelReferenceIdPropertyKey:
                object->as<ViewModelPropertyViewModelBase>()->viewModelReferenceId(value);
                break;
            case ComponentBase::parentIdPropertyKey:
                object->as<ComponentBase>()->parentId(value);
                break;
            case ViewModelInstanceBase::viewModelIdPropertyKey:
                object->as<ViewModelInstanceBase>()->viewModelId(value);
                break;
            case ViewModelPropertyEnumBase::enumIdPropertyKey:
                object->as<ViewModelPropertyEnumBase>()->enumId(value);
                break;
            case ViewModelInstanceViewModelBase::propertyValuePropertyKey:
                object->as<ViewModelInstanceViewModelBase>()->propertyValue(value);
                break;
            case DrawTargetBase::drawableIdPropertyKey:
                object->as<DrawTargetBase>()->drawableId(value);
                break;
            case DrawTargetBase::placementValuePropertyKey:
                object->as<DrawTargetBase>()->placementValue(value);
                break;
            case TargetedConstraintBase::targetIdPropertyKey:
                object->as<TargetedConstraintBase>()->targetId(value);
                break;
            case DistanceConstraintBase::modeValuePropertyKey:
                object->as<DistanceConstraintBase>()->modeValue(value);
                break;
            case TransformSpaceConstraintBase::sourceSpaceValuePropertyKey:
                object->as<TransformSpaceConstraintBase>()->sourceSpaceValue(value);
                break;
            case TransformSpaceConstraintBase::destSpaceValuePropertyKey:
                object->as<TransformSpaceConstraintBase>()->destSpaceValue(value);
                break;
            case TransformComponentConstraintBase::minMaxSpaceValuePropertyKey:
                object->as<TransformComponentConstraintBase>()->minMaxSpaceValue(value);
                break;
            case IKConstraintBase::parentBoneCountPropertyKey:
                object->as<IKConstraintBase>()->parentBoneCount(value);
                break;
            case DrawableBase::blendModeValuePropertyKey:
                object->as<DrawableBase>()->blendModeValue(value);
                break;
            case DrawableBase::drawableFlagsPropertyKey:
                object->as<DrawableBase>()->drawableFlags(value);
                break;
            case NestedArtboardBase::artboardIdPropertyKey:
                object->as<NestedArtboardBase>()->artboardId(value);
                break;
            case NestedArtboardBase::fitPropertyKey:
                object->as<NestedArtboardBase>()->fit(value);
                break;
            case NestedArtboardBase::alignmentPropertyKey:
                object->as<NestedArtboardBase>()->alignment(value);
                break;
            case NestedAnimationBase::animationIdPropertyKey:
                object->as<NestedAnimationBase>()->animationId(value);
                break;
            case SoloBase::activeComponentIdPropertyKey:
                object->as<SoloBase>()->activeComponentId(value);
                break;
            case LayoutComponentStyleBase::layoutFlags0PropertyKey:
                object->as<LayoutComponentStyleBase>()->layoutFlags0(value);
                break;
            case LayoutComponentStyleBase::layoutFlags1PropertyKey:
                object->as<LayoutComponentStyleBase>()->layoutFlags1(value);
                break;
            case LayoutComponentStyleBase::layoutFlags2PropertyKey:
                object->as<LayoutComponentStyleBase>()->layoutFlags2(value);
                break;
            case ListenerFireEventBase::eventIdPropertyKey:
                object->as<ListenerFireEventBase>()->eventId(value);
                break;
            case LayerStateBase::flagsPropertyKey:
                object->as<LayerStateBase>()->flags(value);
                break;
            case ListenerInputChangeBase::inputIdPropertyKey:
                object->as<ListenerInputChangeBase>()->inputId(value);
                break;
            case ListenerInputChangeBase::nestedInputIdPropertyKey:
                object->as<ListenerInputChangeBase>()->nestedInputId(value);
                break;
            case AnimationStateBase::animationIdPropertyKey:
                object->as<AnimationStateBase>()->animationId(value);
                break;
            case NestedInputBase::inputIdPropertyKey:
                object->as<NestedInputBase>()->inputId(value);
                break;
            case KeyedObjectBase::objectIdPropertyKey:
                object->as<KeyedObjectBase>()->objectId(value);
                break;
            case BlendAnimationBase::animationIdPropertyKey:
                object->as<BlendAnimationBase>()->animationId(value);
                break;
            case BlendAnimationDirectBase::inputIdPropertyKey:
                object->as<BlendAnimationDirectBase>()->inputId(value);
                break;
            case BlendAnimationDirectBase::blendSourcePropertyKey:
                object->as<BlendAnimationDirectBase>()->blendSource(value);
                break;
            case TransitionConditionBase::inputIdPropertyKey:
                object->as<TransitionConditionBase>()->inputId(value);
                break;
            case KeyedPropertyBase::propertyKeyPropertyKey:
                object->as<KeyedPropertyBase>()->propertyKey(value);
                break;
            case StateMachineListenerBase::targetIdPropertyKey:
                object->as<StateMachineListenerBase>()->targetId(value);
                break;
            case StateMachineListenerBase::listenerTypeValuePropertyKey:
                object->as<StateMachineListenerBase>()->listenerTypeValue(value);
                break;
            case StateMachineListenerBase::eventIdPropertyKey:
                object->as<StateMachineListenerBase>()->eventId(value);
                break;
            case KeyFrameBase::framePropertyKey:
                object->as<KeyFrameBase>()->frame(value);
                break;
            case InterpolatingKeyFrameBase::interpolationTypePropertyKey:
                object->as<InterpolatingKeyFrameBase>()->interpolationType(value);
                break;
            case InterpolatingKeyFrameBase::interpolatorIdPropertyKey:
                object->as<InterpolatingKeyFrameBase>()->interpolatorId(value);
                break;
            case KeyFrameIdBase::valuePropertyKey:
                object->as<KeyFrameIdBase>()->value(value);
                break;
            case ListenerBoolChangeBase::valuePropertyKey:
                object->as<ListenerBoolChangeBase>()->value(value);
                break;
            case ListenerAlignTargetBase::targetIdPropertyKey:
                object->as<ListenerAlignTargetBase>()->targetId(value);
                break;
            case TransitionValueConditionBase::opValuePropertyKey:
                object->as<TransitionValueConditionBase>()->opValue(value);
                break;
            case StateTransitionBase::stateToIdPropertyKey:
                object->as<StateTransitionBase>()->stateToId(value);
                break;
            case StateTransitionBase::flagsPropertyKey:
                object->as<StateTransitionBase>()->flags(value);
                break;
            case StateTransitionBase::durationPropertyKey:
                object->as<StateTransitionBase>()->duration(value);
                break;
            case StateTransitionBase::exitTimePropertyKey:
                object->as<StateTransitionBase>()->exitTime(value);
                break;
            case StateTransitionBase::interpolationTypePropertyKey:
                object->as<StateTransitionBase>()->interpolationType(value);
                break;
            case StateTransitionBase::interpolatorIdPropertyKey:
                object->as<StateTransitionBase>()->interpolatorId(value);
                break;
            case StateTransitionBase::randomWeightPropertyKey:
                object->as<StateTransitionBase>()->randomWeight(value);
                break;
            case StateMachineFireEventBase::eventIdPropertyKey:
                object->as<StateMachineFireEventBase>()->eventId(value);
                break;
            case StateMachineFireEventBase::occursValuePropertyKey:
                object->as<StateMachineFireEventBase>()->occursValue(value);
                break;
            case LinearAnimationBase::fpsPropertyKey:
                object->as<LinearAnimationBase>()->fps(value);
                break;
            case LinearAnimationBase::durationPropertyKey:
                object->as<LinearAnimationBase>()->duration(value);
                break;
            case LinearAnimationBase::loopValuePropertyKey:
                object->as<LinearAnimationBase>()->loopValue(value);
                break;
            case LinearAnimationBase::workStartPropertyKey:
                object->as<LinearAnimationBase>()->workStart(value);
                break;
            case LinearAnimationBase::workEndPropertyKey:
                object->as<LinearAnimationBase>()->workEnd(value);
                break;
            case ElasticInterpolatorBase::easingValuePropertyKey:
                object->as<ElasticInterpolatorBase>()->easingValue(value);
                break;
            case BlendState1DBase::inputIdPropertyKey:
                object->as<BlendState1DBase>()->inputId(value);
                break;
            case BlendStateTransitionBase::exitBlendAnimationIdPropertyKey:
                object->as<BlendStateTransitionBase>()->exitBlendAnimationId(value);
                break;
            case StrokeBase::capPropertyKey:
                object->as<StrokeBase>()->cap(value);
                break;
            case StrokeBase::joinPropertyKey:
                object->as<StrokeBase>()->join(value);
                break;
            case TrimPathBase::modeValuePropertyKey:
                object->as<TrimPathBase>()->modeValue(value);
                break;
            case FillBase::fillRulePropertyKey:
                object->as<FillBase>()->fillRule(value);
                break;
            case PathBase::pathFlagsPropertyKey:
                object->as<PathBase>()->pathFlags(value);
                break;
            case ClippingShapeBase::sourceIdPropertyKey:
                object->as<ClippingShapeBase>()->sourceId(value);
                break;
            case ClippingShapeBase::fillRulePropertyKey:
                object->as<ClippingShapeBase>()->fillRule(value);
                break;
            case PolygonBase::pointsPropertyKey:
                object->as<PolygonBase>()->points(value);
                break;
            case ImageBase::assetIdPropertyKey:
                object->as<ImageBase>()->assetId(value);
                break;
            case DrawRulesBase::drawTargetIdPropertyKey:
                object->as<DrawRulesBase>()->drawTargetId(value);
                break;
            case LayoutComponentBase::styleIdPropertyKey:
                object->as<LayoutComponentBase>()->styleId(value);
                break;
            case ArtboardBase::defaultStateMachineIdPropertyKey:
                object->as<ArtboardBase>()->defaultStateMachineId(value);
                break;
            case ArtboardBase::viewModelIdPropertyKey:
                object->as<ArtboardBase>()->viewModelId(value);
                break;
            case JoystickBase::xIdPropertyKey:
                object->as<JoystickBase>()->xId(value);
                break;
            case JoystickBase::yIdPropertyKey:
                object->as<JoystickBase>()->yId(value);
                break;
            case JoystickBase::joystickFlagsPropertyKey:
                object->as<JoystickBase>()->joystickFlags(value);
                break;
            case JoystickBase::handleSourceIdPropertyKey:
                object->as<JoystickBase>()->handleSourceId(value);
                break;
            case OpenUrlEventBase::targetValuePropertyKey:
                object->as<OpenUrlEventBase>()->targetValue(value);
                break;
            case DataBindBase::targetIdPropertyKey:
                object->as<DataBindBase>()->targetId(value);
                break;
            case DataBindBase::propertyKeyPropertyKey:
                object->as<DataBindBase>()->propertyKey(value);
                break;
            case DataBindBase::modeValuePropertyKey:
                object->as<DataBindBase>()->modeValue(value);
                break;
            case WeightBase::valuesPropertyKey:
                object->as<WeightBase>()->values(value);
                break;
            case WeightBase::indicesPropertyKey:
                object->as<WeightBase>()->indices(value);
                break;
            case TendonBase::boneIdPropertyKey:
                object->as<TendonBase>()->boneId(value);
                break;
            case CubicWeightBase::inValuesPropertyKey:
                object->as<CubicWeightBase>()->inValues(value);
                break;
            case CubicWeightBase::inIndicesPropertyKey:
                object->as<CubicWeightBase>()->inIndices(value);
                break;
            case CubicWeightBase::outValuesPropertyKey:
                object->as<CubicWeightBase>()->outValues(value);
                break;
            case CubicWeightBase::outIndicesPropertyKey:
                object->as<CubicWeightBase>()->outIndices(value);
                break;
            case TextModifierRangeBase::unitsValuePropertyKey:
                object->as<TextModifierRangeBase>()->unitsValue(value);
                break;
            case TextModifierRangeBase::typeValuePropertyKey:
                object->as<TextModifierRangeBase>()->typeValue(value);
                break;
            case TextModifierRangeBase::modeValuePropertyKey:
                object->as<TextModifierRangeBase>()->modeValue(value);
                break;
            case TextModifierRangeBase::runIdPropertyKey:
                object->as<TextModifierRangeBase>()->runId(value);
                break;
            case TextStyleFeatureBase::tagPropertyKey:
                object->as<TextStyleFeatureBase>()->tag(value);
                break;
            case TextStyleFeatureBase::featureValuePropertyKey:
                object->as<TextStyleFeatureBase>()->featureValue(value);
                break;
            case TextVariationModifierBase::axisTagPropertyKey:
                object->as<TextVariationModifierBase>()->axisTag(value);
                break;
            case TextModifierGroupBase::modifierFlagsPropertyKey:
                object->as<TextModifierGroupBase>()->modifierFlags(value);
                break;
            case TextStyleBase::fontAssetIdPropertyKey:
                object->as<TextStyleBase>()->fontAssetId(value);
                break;
            case TextStyleAxisBase::tagPropertyKey:
                object->as<TextStyleAxisBase>()->tag(value);
                break;
            case TextBase::alignValuePropertyKey:
                object->as<TextBase>()->alignValue(value);
                break;
            case TextBase::sizingValuePropertyKey:
                object->as<TextBase>()->sizingValue(value);
                break;
            case TextBase::overflowValuePropertyKey:
                object->as<TextBase>()->overflowValue(value);
                break;
            case TextBase::originValuePropertyKey:
                object->as<TextBase>()->originValue(value);
                break;
            case TextValueRunBase::styleIdPropertyKey:
                object->as<TextValueRunBase>()->styleId(value);
                break;
            case FileAssetBase::assetIdPropertyKey:
                object->as<FileAssetBase>()->assetId(value);
                break;
            case AudioEventBase::assetIdPropertyKey:
                object->as<AudioEventBase>()->assetId(value);
                break;
        }
    }
    static void setColor(Core* object, int propertyKey, int value)
    {
        switch (propertyKey)
        {
            case ViewModelInstanceColorBase::propertyValuePropertyKey:
                object->as<ViewModelInstanceColorBase>()->propertyValue(value);
                break;
            case KeyFrameColorBase::valuePropertyKey:
                object->as<KeyFrameColorBase>()->value(value);
                break;
            case SolidColorBase::colorValuePropertyKey:
                object->as<SolidColorBase>()->colorValue(value);
                break;
            case GradientStopBase::colorValuePropertyKey:
                object->as<GradientStopBase>()->colorValue(value);
                break;
        }
    }
    static void setString(Core* object, int propertyKey, std::string value)
    {
        switch (propertyKey)
        {
            case ViewModelComponentBase::namePropertyKey:
                object->as<ViewModelComponentBase>()->name(value);
                break;
            case ViewModelInstanceStringBase::propertyValuePropertyKey:
                object->as<ViewModelInstanceStringBase>()->propertyValue(value);
                break;
            case ComponentBase::namePropertyKey:
                object->as<ComponentBase>()->name(value);
                break;
            case DataEnumValueBase::keyPropertyKey:
                object->as<DataEnumValueBase>()->key(value);
                break;
            case DataEnumValueBase::valuePropertyKey:
                object->as<DataEnumValueBase>()->value(value);
                break;
            case AnimationBase::namePropertyKey:
                object->as<AnimationBase>()->name(value);
                break;
            case StateMachineComponentBase::namePropertyKey:
                object->as<StateMachineComponentBase>()->name(value);
                break;
            case KeyFrameStringBase::valuePropertyKey:
                object->as<KeyFrameStringBase>()->value(value);
                break;
            case OpenUrlEventBase::urlPropertyKey:
                object->as<OpenUrlEventBase>()->url(value);
                break;
            case TextValueRunBase::textPropertyKey:
                object->as<TextValueRunBase>()->text(value);
                break;
            case CustomPropertyStringBase::propertyValuePropertyKey:
                object->as<CustomPropertyStringBase>()->propertyValue(value);
                break;
            case AssetBase::namePropertyKey:
                object->as<AssetBase>()->name(value);
                break;
            case FileAssetBase::cdnBaseUrlPropertyKey:
                object->as<FileAssetBase>()->cdnBaseUrl(value);
                break;
        }
    }
    static void setDouble(Core* object, int propertyKey, float value)
    {
        switch (propertyKey)
        {
            case ViewModelInstanceNumberBase::propertyValuePropertyKey:
                object->as<ViewModelInstanceNumberBase>()->propertyValue(value);
                break;
            case CustomPropertyNumberBase::propertyValuePropertyKey:
                object->as<CustomPropertyNumberBase>()->propertyValue(value);
                break;
            case ConstraintBase::strengthPropertyKey:
                object->as<ConstraintBase>()->strength(value);
                break;
            case DistanceConstraintBase::distancePropertyKey:
                object->as<DistanceConstraintBase>()->distance(value);
                break;
            case TransformComponentConstraintBase::copyFactorPropertyKey:
                object->as<TransformComponentConstraintBase>()->copyFactor(value);
                break;
            case TransformComponentConstraintBase::minValuePropertyKey:
                object->as<TransformComponentConstraintBase>()->minValue(value);
                break;
            case TransformComponentConstraintBase::maxValuePropertyKey:
                object->as<TransformComponentConstraintBase>()->maxValue(value);
                break;
            case TransformComponentConstraintYBase::copyFactorYPropertyKey:
                object->as<TransformComponentConstraintYBase>()->copyFactorY(value);
                break;
            case TransformComponentConstraintYBase::minValueYPropertyKey:
                object->as<TransformComponentConstraintYBase>()->minValueY(value);
                break;
            case TransformComponentConstraintYBase::maxValueYPropertyKey:
                object->as<TransformComponentConstraintYBase>()->maxValueY(value);
                break;
            case FollowPathConstraintBase::distancePropertyKey:
                object->as<FollowPathConstraintBase>()->distance(value);
                break;
            case TransformConstraintBase::originXPropertyKey:
                object->as<TransformConstraintBase>()->originX(value);
                break;
            case TransformConstraintBase::originYPropertyKey:
                object->as<TransformConstraintBase>()->originY(value);
                break;
            case WorldTransformComponentBase::opacityPropertyKey:
                object->as<WorldTransformComponentBase>()->opacity(value);
                break;
            case TransformComponentBase::rotationPropertyKey:
                object->as<TransformComponentBase>()->rotation(value);
                break;
            case TransformComponentBase::scaleXPropertyKey:
                object->as<TransformComponentBase>()->scaleX(value);
                break;
            case TransformComponentBase::scaleYPropertyKey:
                object->as<TransformComponentBase>()->scaleY(value);
                break;
            case NodeBase::xPropertyKey:
                object->as<NodeBase>()->x(value);
                break;
            case NodeBase::yPropertyKey:
                object->as<NodeBase>()->y(value);
                break;
            case LayoutComponentStyleBase::gapHorizontalPropertyKey:
                object->as<LayoutComponentStyleBase>()->gapHorizontal(value);
                break;
            case LayoutComponentStyleBase::gapVerticalPropertyKey:
                object->as<LayoutComponentStyleBase>()->gapVertical(value);
                break;
            case LayoutComponentStyleBase::maxWidthPropertyKey:
                object->as<LayoutComponentStyleBase>()->maxWidth(value);
                break;
            case LayoutComponentStyleBase::maxHeightPropertyKey:
                object->as<LayoutComponentStyleBase>()->maxHeight(value);
                break;
            case LayoutComponentStyleBase::minWidthPropertyKey:
                object->as<LayoutComponentStyleBase>()->minWidth(value);
                break;
            case LayoutComponentStyleBase::minHeightPropertyKey:
                object->as<LayoutComponentStyleBase>()->minHeight(value);
                break;
            case LayoutComponentStyleBase::borderLeftPropertyKey:
                object->as<LayoutComponentStyleBase>()->borderLeft(value);
                break;
            case LayoutComponentStyleBase::borderRightPropertyKey:
                object->as<LayoutComponentStyleBase>()->borderRight(value);
                break;
            case LayoutComponentStyleBase::borderTopPropertyKey:
                object->as<LayoutComponentStyleBase>()->borderTop(value);
                break;
            case LayoutComponentStyleBase::borderBottomPropertyKey:
                object->as<LayoutComponentStyleBase>()->borderBottom(value);
                break;
            case LayoutComponentStyleBase::marginLeftPropertyKey:
                object->as<LayoutComponentStyleBase>()->marginLeft(value);
                break;
            case LayoutComponentStyleBase::marginRightPropertyKey:
                object->as<LayoutComponentStyleBase>()->marginRight(value);
                break;
            case LayoutComponentStyleBase::marginTopPropertyKey:
                object->as<LayoutComponentStyleBase>()->marginTop(value);
                break;
            case LayoutComponentStyleBase::marginBottomPropertyKey:
                object->as<LayoutComponentStyleBase>()->marginBottom(value);
                break;
            case LayoutComponentStyleBase::paddingLeftPropertyKey:
                object->as<LayoutComponentStyleBase>()->paddingLeft(value);
                break;
            case LayoutComponentStyleBase::paddingRightPropertyKey:
                object->as<LayoutComponentStyleBase>()->paddingRight(value);
                break;
            case LayoutComponentStyleBase::paddingTopPropertyKey:
                object->as<LayoutComponentStyleBase>()->paddingTop(value);
                break;
            case LayoutComponentStyleBase::paddingBottomPropertyKey:
                object->as<LayoutComponentStyleBase>()->paddingBottom(value);
                break;
            case LayoutComponentStyleBase::positionLeftPropertyKey:
                object->as<LayoutComponentStyleBase>()->positionLeft(value);
                break;
            case LayoutComponentStyleBase::positionRightPropertyKey:
                object->as<LayoutComponentStyleBase>()->positionRight(value);
                break;
            case LayoutComponentStyleBase::positionTopPropertyKey:
                object->as<LayoutComponentStyleBase>()->positionTop(value);
                break;
            case LayoutComponentStyleBase::positionBottomPropertyKey:
                object->as<LayoutComponentStyleBase>()->positionBottom(value);
                break;
            case LayoutComponentStyleBase::flexPropertyKey:
                object->as<LayoutComponentStyleBase>()->flex(value);
                break;
            case LayoutComponentStyleBase::flexGrowPropertyKey:
                object->as<LayoutComponentStyleBase>()->flexGrow(value);
                break;
            case LayoutComponentStyleBase::flexShrinkPropertyKey:
                object->as<LayoutComponentStyleBase>()->flexShrink(value);
                break;
            case LayoutComponentStyleBase::flexBasisPropertyKey:
                object->as<LayoutComponentStyleBase>()->flexBasis(value);
                break;
            case LayoutComponentStyleBase::aspectRatioPropertyKey:
                object->as<LayoutComponentStyleBase>()->aspectRatio(value);
                break;
            case NestedLinearAnimationBase::mixPropertyKey:
                object->as<NestedLinearAnimationBase>()->mix(value);
                break;
            case NestedSimpleAnimationBase::speedPropertyKey:
                object->as<NestedSimpleAnimationBase>()->speed(value);
                break;
            case AdvanceableStateBase::speedPropertyKey:
                object->as<AdvanceableStateBase>()->speed(value);
                break;
            case BlendAnimationDirectBase::mixValuePropertyKey:
                object->as<BlendAnimationDirectBase>()->mixValue(value);
                break;
            case StateMachineNumberBase::valuePropertyKey:
                object->as<StateMachineNumberBase>()->value(value);
                break;
            case CubicInterpolatorBase::x1PropertyKey:
                object->as<CubicInterpolatorBase>()->x1(value);
                break;
            case CubicInterpolatorBase::y1PropertyKey:
                object->as<CubicInterpolatorBase>()->y1(value);
                break;
            case CubicInterpolatorBase::x2PropertyKey:
                object->as<CubicInterpolatorBase>()->x2(value);
                break;
            case CubicInterpolatorBase::y2PropertyKey:
                object->as<CubicInterpolatorBase>()->y2(value);
                break;
            case TransitionNumberConditionBase::valuePropertyKey:
                object->as<TransitionNumberConditionBase>()->value(value);
                break;
            case CubicInterpolatorComponentBase::x1PropertyKey:
                object->as<CubicInterpolatorComponentBase>()->x1(value);
                break;
            case CubicInterpolatorComponentBase::y1PropertyKey:
                object->as<CubicInterpolatorComponentBase>()->y1(value);
                break;
            case CubicInterpolatorComponentBase::x2PropertyKey:
                object->as<CubicInterpolatorComponentBase>()->x2(value);
                break;
            case CubicInterpolatorComponentBase::y2PropertyKey:
                object->as<CubicInterpolatorComponentBase>()->y2(value);
                break;
            case ListenerNumberChangeBase::valuePropertyKey:
                object->as<ListenerNumberChangeBase>()->value(value);
                break;
            case KeyFrameDoubleBase::valuePropertyKey:
                object->as<KeyFrameDoubleBase>()->value(value);
                break;
            case LinearAnimationBase::speedPropertyKey:
                object->as<LinearAnimationBase>()->speed(value);
                break;
            case ElasticInterpolatorBase::amplitudePropertyKey:
                object->as<ElasticInterpolatorBase>()->amplitude(value);
                break;
            case ElasticInterpolatorBase::periodPropertyKey:
                object->as<ElasticInterpolatorBase>()->period(value);
                break;
            case NestedNumberBase::nestedValuePropertyKey:
                object->as<NestedNumberBase>()->nestedValue(value);
                break;
            case NestedRemapAnimationBase::timePropertyKey:
                object->as<NestedRemapAnimationBase>()->time(value);
                break;
            case BlendAnimation1DBase::valuePropertyKey:
                object->as<BlendAnimation1DBase>()->value(value);
                break;
            case LinearGradientBase::startXPropertyKey:
                object->as<LinearGradientBase>()->startX(value);
                break;
            case LinearGradientBase::startYPropertyKey:
                object->as<LinearGradientBase>()->startY(value);
                break;
            case LinearGradientBase::endXPropertyKey:
                object->as<LinearGradientBase>()->endX(value);
                break;
            case LinearGradientBase::endYPropertyKey:
                object->as<LinearGradientBase>()->endY(value);
                break;
            case LinearGradientBase::opacityPropertyKey:
                object->as<LinearGradientBase>()->opacity(value);
                break;
            case StrokeBase::thicknessPropertyKey:
                object->as<StrokeBase>()->thickness(value);
                break;
            case GradientStopBase::positionPropertyKey:
                object->as<GradientStopBase>()->position(value);
                break;
            case TrimPathBase::startPropertyKey:
                object->as<TrimPathBase>()->start(value);
                break;
            case TrimPathBase::endPropertyKey:
                object->as<TrimPathBase>()->end(value);
                break;
            case TrimPathBase::offsetPropertyKey:
                object->as<TrimPathBase>()->offset(value);
                break;
            case VertexBase::xPropertyKey:
                object->as<VertexBase>()->x(value);
                break;
            case VertexBase::yPropertyKey:
                object->as<VertexBase>()->y(value);
                break;
            case MeshVertexBase::uPropertyKey:
                object->as<MeshVertexBase>()->u(value);
                break;
            case MeshVertexBase::vPropertyKey:
                object->as<MeshVertexBase>()->v(value);
                break;
            case StraightVertexBase::radiusPropertyKey:
                object->as<StraightVertexBase>()->radius(value);
                break;
            case CubicAsymmetricVertexBase::rotationPropertyKey:
                object->as<CubicAsymmetricVertexBase>()->rotation(value);
                break;
            case CubicAsymmetricVertexBase::inDistancePropertyKey:
                object->as<CubicAsymmetricVertexBase>()->inDistance(value);
                break;
            case CubicAsymmetricVertexBase::outDistancePropertyKey:
                object->as<CubicAsymmetricVertexBase>()->outDistance(value);
                break;
            case ParametricPathBase::widthPropertyKey:
                object->as<ParametricPathBase>()->width(value);
                break;
            case ParametricPathBase::heightPropertyKey:
                object->as<ParametricPathBase>()->height(value);
                break;
            case ParametricPathBase::originXPropertyKey:
                object->as<ParametricPathBase>()->originX(value);
                break;
            case ParametricPathBase::originYPropertyKey:
                object->as<ParametricPathBase>()->originY(value);
                break;
            case RectangleBase::cornerRadiusTLPropertyKey:
                object->as<RectangleBase>()->cornerRadiusTL(value);
                break;
            case RectangleBase::cornerRadiusTRPropertyKey:
                object->as<RectangleBase>()->cornerRadiusTR(value);
                break;
            case RectangleBase::cornerRadiusBLPropertyKey:
                object->as<RectangleBase>()->cornerRadiusBL(value);
                break;
            case RectangleBase::cornerRadiusBRPropertyKey:
                object->as<RectangleBase>()->cornerRadiusBR(value);
                break;
            case CubicMirroredVertexBase::rotationPropertyKey:
                object->as<CubicMirroredVertexBase>()->rotation(value);
                break;
            case CubicMirroredVertexBase::distancePropertyKey:
                object->as<CubicMirroredVertexBase>()->distance(value);
                break;
            case PolygonBase::cornerRadiusPropertyKey:
                object->as<PolygonBase>()->cornerRadius(value);
                break;
            case StarBase::innerRadiusPropertyKey:
                object->as<StarBase>()->innerRadius(value);
                break;
            case ImageBase::originXPropertyKey:
                object->as<ImageBase>()->originX(value);
                break;
            case ImageBase::originYPropertyKey:
                object->as<ImageBase>()->originY(value);
                break;
            case CubicDetachedVertexBase::inRotationPropertyKey:
                object->as<CubicDetachedVertexBase>()->inRotation(value);
                break;
            case CubicDetachedVertexBase::inDistancePropertyKey:
                object->as<CubicDetachedVertexBase>()->inDistance(value);
                break;
            case CubicDetachedVertexBase::outRotationPropertyKey:
                object->as<CubicDetachedVertexBase>()->outRotation(value);
                break;
            case CubicDetachedVertexBase::outDistancePropertyKey:
                object->as<CubicDetachedVertexBase>()->outDistance(value);
                break;
            case LayoutComponentBase::widthPropertyKey:
                object->as<LayoutComponentBase>()->width(value);
                break;
            case LayoutComponentBase::heightPropertyKey:
                object->as<LayoutComponentBase>()->height(value);
                break;
            case ArtboardBase::xPropertyKey:
                object->as<ArtboardBase>()->x(value);
                break;
            case ArtboardBase::yPropertyKey:
                object->as<ArtboardBase>()->y(value);
                break;
            case ArtboardBase::originXPropertyKey:
                object->as<ArtboardBase>()->originX(value);
                break;
            case ArtboardBase::originYPropertyKey:
                object->as<ArtboardBase>()->originY(value);
                break;
            case JoystickBase::xPropertyKey:
                object->as<JoystickBase>()->x(value);
                break;
            case JoystickBase::yPropertyKey:
                object->as<JoystickBase>()->y(value);
                break;
            case JoystickBase::posXPropertyKey:
                object->as<JoystickBase>()->posX(value);
                break;
            case JoystickBase::posYPropertyKey:
                object->as<JoystickBase>()->posY(value);
                break;
            case JoystickBase::originXPropertyKey:
                object->as<JoystickBase>()->originX(value);
                break;
            case JoystickBase::originYPropertyKey:
                object->as<JoystickBase>()->originY(value);
                break;
            case JoystickBase::widthPropertyKey:
                object->as<JoystickBase>()->width(value);
                break;
            case JoystickBase::heightPropertyKey:
                object->as<JoystickBase>()->height(value);
                break;
            case BoneBase::lengthPropertyKey:
                object->as<BoneBase>()->length(value);
                break;
            case RootBoneBase::xPropertyKey:
                object->as<RootBoneBase>()->x(value);
                break;
            case RootBoneBase::yPropertyKey:
                object->as<RootBoneBase>()->y(value);
                break;
            case SkinBase::xxPropertyKey:
                object->as<SkinBase>()->xx(value);
                break;
            case SkinBase::yxPropertyKey:
                object->as<SkinBase>()->yx(value);
                break;
            case SkinBase::xyPropertyKey:
                object->as<SkinBase>()->xy(value);
                break;
            case SkinBase::yyPropertyKey:
                object->as<SkinBase>()->yy(value);
                break;
            case SkinBase::txPropertyKey:
                object->as<SkinBase>()->tx(value);
                break;
            case SkinBase::tyPropertyKey:
                object->as<SkinBase>()->ty(value);
                break;
            case TendonBase::xxPropertyKey:
                object->as<TendonBase>()->xx(value);
                break;
            case TendonBase::yxPropertyKey:
                object->as<TendonBase>()->yx(value);
                break;
            case TendonBase::xyPropertyKey:
                object->as<TendonBase>()->xy(value);
                break;
            case TendonBase::yyPropertyKey:
                object->as<TendonBase>()->yy(value);
                break;
            case TendonBase::txPropertyKey:
                object->as<TendonBase>()->tx(value);
                break;
            case TendonBase::tyPropertyKey:
                object->as<TendonBase>()->ty(value);
                break;
            case TextModifierRangeBase::modifyFromPropertyKey:
                object->as<TextModifierRangeBase>()->modifyFrom(value);
                break;
            case TextModifierRangeBase::modifyToPropertyKey:
                object->as<TextModifierRangeBase>()->modifyTo(value);
                break;
            case TextModifierRangeBase::strengthPropertyKey:
                object->as<TextModifierRangeBase>()->strength(value);
                break;
            case TextModifierRangeBase::falloffFromPropertyKey:
                object->as<TextModifierRangeBase>()->falloffFrom(value);
                break;
            case TextModifierRangeBase::falloffToPropertyKey:
                object->as<TextModifierRangeBase>()->falloffTo(value);
                break;
            case TextModifierRangeBase::offsetPropertyKey:
                object->as<TextModifierRangeBase>()->offset(value);
                break;
            case TextVariationModifierBase::axisValuePropertyKey:
                object->as<TextVariationModifierBase>()->axisValue(value);
                break;
            case TextModifierGroupBase::originXPropertyKey:
                object->as<TextModifierGroupBase>()->originX(value);
                break;
            case TextModifierGroupBase::originYPropertyKey:
                object->as<TextModifierGroupBase>()->originY(value);
                break;
            case TextModifierGroupBase::opacityPropertyKey:
                object->as<TextModifierGroupBase>()->opacity(value);
                break;
            case TextModifierGroupBase::xPropertyKey:
                object->as<TextModifierGroupBase>()->x(value);
                break;
            case TextModifierGroupBase::yPropertyKey:
                object->as<TextModifierGroupBase>()->y(value);
                break;
            case TextModifierGroupBase::rotationPropertyKey:
                object->as<TextModifierGroupBase>()->rotation(value);
                break;
            case TextModifierGroupBase::scaleXPropertyKey:
                object->as<TextModifierGroupBase>()->scaleX(value);
                break;
            case TextModifierGroupBase::scaleYPropertyKey:
                object->as<TextModifierGroupBase>()->scaleY(value);
                break;
            case TextStyleBase::fontSizePropertyKey:
                object->as<TextStyleBase>()->fontSize(value);
                break;
            case TextStyleBase::lineHeightPropertyKey:
                object->as<TextStyleBase>()->lineHeight(value);
                break;
            case TextStyleBase::letterSpacingPropertyKey:
                object->as<TextStyleBase>()->letterSpacing(value);
                break;
            case TextStyleAxisBase::axisValuePropertyKey:
                object->as<TextStyleAxisBase>()->axisValue(value);
                break;
            case TextBase::widthPropertyKey:
                object->as<TextBase>()->width(value);
                break;
            case TextBase::heightPropertyKey:
                object->as<TextBase>()->height(value);
                break;
            case TextBase::originXPropertyKey:
                object->as<TextBase>()->originX(value);
                break;
            case TextBase::originYPropertyKey:
                object->as<TextBase>()->originY(value);
                break;
            case TextBase::paragraphSpacingPropertyKey:
                object->as<TextBase>()->paragraphSpacing(value);
                break;
            case DrawableAssetBase::heightPropertyKey:
                object->as<DrawableAssetBase>()->height(value);
                break;
            case DrawableAssetBase::widthPropertyKey:
                object->as<DrawableAssetBase>()->width(value);
                break;
            case ExportAudioBase::volumePropertyKey:
                object->as<ExportAudioBase>()->volume(value);
                break;
        }
    }
    static void setCallback(Core* object, int propertyKey, CallbackData value)
    {
        switch (propertyKey)
        {
            case NestedTriggerBase::firePropertyKey:
                object->as<NestedTriggerBase>()->fire(value);
                break;
            case EventBase::triggerPropertyKey:
                object->as<EventBase>()->trigger(value);
                break;
        }
    }
    static bool getBool(Core* object, int propertyKey)
    {
        switch (propertyKey)
        {
            case ViewModelInstanceListItemBase::useLinkedArtboardPropertyKey:
                return object->as<ViewModelInstanceListItemBase>()->useLinkedArtboard();
            case TransformComponentConstraintBase::offsetPropertyKey:
                return object->as<TransformComponentConstraintBase>()->offset();
            case TransformComponentConstraintBase::doesCopyPropertyKey:
                return object->as<TransformComponentConstraintBase>()->doesCopy();
            case TransformComponentConstraintBase::minPropertyKey:
                return object->as<TransformComponentConstraintBase>()->min();
            case TransformComponentConstraintBase::maxPropertyKey:
                return object->as<TransformComponentConstraintBase>()->max();
            case TransformComponentConstraintYBase::doesCopyYPropertyKey:
                return object->as<TransformComponentConstraintYBase>()->doesCopyY();
            case TransformComponentConstraintYBase::minYPropertyKey:
                return object->as<TransformComponentConstraintYBase>()->minY();
            case TransformComponentConstraintYBase::maxYPropertyKey:
                return object->as<TransformComponentConstraintYBase>()->maxY();
            case IKConstraintBase::invertDirectionPropertyKey:
                return object->as<IKConstraintBase>()->invertDirection();
            case FollowPathConstraintBase::orientPropertyKey:
                return object->as<FollowPathConstraintBase>()->orient();
            case FollowPathConstraintBase::offsetPropertyKey:
                return object->as<FollowPathConstraintBase>()->offset();
            case NestedSimpleAnimationBase::isPlayingPropertyKey:
                return object->as<NestedSimpleAnimationBase>()->isPlaying();
            case KeyFrameBoolBase::valuePropertyKey:
                return object->as<KeyFrameBoolBase>()->value();
            case ListenerAlignTargetBase::preserveOffsetPropertyKey:
                return object->as<ListenerAlignTargetBase>()->preserveOffset();
            case NestedBoolBase::nestedValuePropertyKey:
                return object->as<NestedBoolBase>()->nestedValue();
            case LinearAnimationBase::enableWorkAreaPropertyKey:
                return object->as<LinearAnimationBase>()->enableWorkArea();
            case LinearAnimationBase::quantizePropertyKey:
                return object->as<LinearAnimationBase>()->quantize();
            case StateMachineBoolBase::valuePropertyKey:
                return object->as<StateMachineBoolBase>()->value();
            case ShapePaintBase::isVisiblePropertyKey:
                return object->as<ShapePaintBase>()->isVisible();
            case StrokeBase::transformAffectsStrokePropertyKey:
                return object->as<StrokeBase>()->transformAffectsStroke();
            case PointsPathBase::isClosedPropertyKey:
                return object->as<PointsPathBase>()->isClosed();
            case RectangleBase::linkCornerRadiusPropertyKey:
                return object->as<RectangleBase>()->linkCornerRadius();
            case ClippingShapeBase::isVisiblePropertyKey:
                return object->as<ClippingShapeBase>()->isVisible();
            case CustomPropertyBooleanBase::propertyValuePropertyKey:
                return object->as<CustomPropertyBooleanBase>()->propertyValue();
            case LayoutComponentBase::clipPropertyKey:
                return object->as<LayoutComponentBase>()->clip();
            case TextModifierRangeBase::clampPropertyKey:
                return object->as<TextModifierRangeBase>()->clamp();
        }
        return false;
    }
    static uint32_t getUint(Core* object, int propertyKey)
    {
        switch (propertyKey)
        {
            case ViewModelInstanceListItemBase::viewModelIdPropertyKey:
                return object->as<ViewModelInstanceListItemBase>()->viewModelId();
            case ViewModelInstanceListItemBase::viewModelInstanceIdPropertyKey:
                return object->as<ViewModelInstanceListItemBase>()->viewModelInstanceId();
            case ViewModelInstanceListItemBase::artboardIdPropertyKey:
                return object->as<ViewModelInstanceListItemBase>()->artboardId();
            case ViewModelInstanceValueBase::viewModelPropertyIdPropertyKey:
                return object->as<ViewModelInstanceValueBase>()->viewModelPropertyId();
            case ViewModelInstanceEnumBase::propertyValuePropertyKey:
                return object->as<ViewModelInstanceEnumBase>()->propertyValue();
            case ViewModelBase::defaultInstanceIdPropertyKey:
                return object->as<ViewModelBase>()->defaultInstanceId();
            case ViewModelPropertyViewModelBase::viewModelReferenceIdPropertyKey:
                return object->as<ViewModelPropertyViewModelBase>()->viewModelReferenceId();
            case ComponentBase::parentIdPropertyKey:
                return object->as<ComponentBase>()->parentId();
            case ViewModelInstanceBase::viewModelIdPropertyKey:
                return object->as<ViewModelInstanceBase>()->viewModelId();
            case ViewModelPropertyEnumBase::enumIdPropertyKey:
                return object->as<ViewModelPropertyEnumBase>()->enumId();
            case ViewModelInstanceViewModelBase::propertyValuePropertyKey:
                return object->as<ViewModelInstanceViewModelBase>()->propertyValue();
            case DrawTargetBase::drawableIdPropertyKey:
                return object->as<DrawTargetBase>()->drawableId();
            case DrawTargetBase::placementValuePropertyKey:
                return object->as<DrawTargetBase>()->placementValue();
            case TargetedConstraintBase::targetIdPropertyKey:
                return object->as<TargetedConstraintBase>()->targetId();
            case DistanceConstraintBase::modeValuePropertyKey:
                return object->as<DistanceConstraintBase>()->modeValue();
            case TransformSpaceConstraintBase::sourceSpaceValuePropertyKey:
                return object->as<TransformSpaceConstraintBase>()->sourceSpaceValue();
            case TransformSpaceConstraintBase::destSpaceValuePropertyKey:
                return object->as<TransformSpaceConstraintBase>()->destSpaceValue();
            case TransformComponentConstraintBase::minMaxSpaceValuePropertyKey:
                return object->as<TransformComponentConstraintBase>()->minMaxSpaceValue();
            case IKConstraintBase::parentBoneCountPropertyKey:
                return object->as<IKConstraintBase>()->parentBoneCount();
            case DrawableBase::blendModeValuePropertyKey:
                return object->as<DrawableBase>()->blendModeValue();
            case DrawableBase::drawableFlagsPropertyKey:
                return object->as<DrawableBase>()->drawableFlags();
            case NestedArtboardBase::artboardIdPropertyKey:
                return object->as<NestedArtboardBase>()->artboardId();
            case NestedArtboardBase::fitPropertyKey:
                return object->as<NestedArtboardBase>()->fit();
            case NestedArtboardBase::alignmentPropertyKey:
                return object->as<NestedArtboardBase>()->alignment();
            case NestedAnimationBase::animationIdPropertyKey:
                return object->as<NestedAnimationBase>()->animationId();
            case SoloBase::activeComponentIdPropertyKey:
                return object->as<SoloBase>()->activeComponentId();
            case LayoutComponentStyleBase::layoutFlags0PropertyKey:
                return object->as<LayoutComponentStyleBase>()->layoutFlags0();
            case LayoutComponentStyleBase::layoutFlags1PropertyKey:
                return object->as<LayoutComponentStyleBase>()->layoutFlags1();
            case LayoutComponentStyleBase::layoutFlags2PropertyKey:
                return object->as<LayoutComponentStyleBase>()->layoutFlags2();
            case ListenerFireEventBase::eventIdPropertyKey:
                return object->as<ListenerFireEventBase>()->eventId();
            case LayerStateBase::flagsPropertyKey:
                return object->as<LayerStateBase>()->flags();
            case ListenerInputChangeBase::inputIdPropertyKey:
                return object->as<ListenerInputChangeBase>()->inputId();
            case ListenerInputChangeBase::nestedInputIdPropertyKey:
                return object->as<ListenerInputChangeBase>()->nestedInputId();
            case AnimationStateBase::animationIdPropertyKey:
                return object->as<AnimationStateBase>()->animationId();
            case NestedInputBase::inputIdPropertyKey:
                return object->as<NestedInputBase>()->inputId();
            case KeyedObjectBase::objectIdPropertyKey:
                return object->as<KeyedObjectBase>()->objectId();
            case BlendAnimationBase::animationIdPropertyKey:
                return object->as<BlendAnimationBase>()->animationId();
            case BlendAnimationDirectBase::inputIdPropertyKey:
                return object->as<BlendAnimationDirectBase>()->inputId();
            case BlendAnimationDirectBase::blendSourcePropertyKey:
                return object->as<BlendAnimationDirectBase>()->blendSource();
            case TransitionConditionBase::inputIdPropertyKey:
                return object->as<TransitionConditionBase>()->inputId();
            case KeyedPropertyBase::propertyKeyPropertyKey:
                return object->as<KeyedPropertyBase>()->propertyKey();
            case StateMachineListenerBase::targetIdPropertyKey:
                return object->as<StateMachineListenerBase>()->targetId();
            case StateMachineListenerBase::listenerTypeValuePropertyKey:
                return object->as<StateMachineListenerBase>()->listenerTypeValue();
            case StateMachineListenerBase::eventIdPropertyKey:
                return object->as<StateMachineListenerBase>()->eventId();
            case KeyFrameBase::framePropertyKey:
                return object->as<KeyFrameBase>()->frame();
            case InterpolatingKeyFrameBase::interpolationTypePropertyKey:
                return object->as<InterpolatingKeyFrameBase>()->interpolationType();
            case InterpolatingKeyFrameBase::interpolatorIdPropertyKey:
                return object->as<InterpolatingKeyFrameBase>()->interpolatorId();
            case KeyFrameIdBase::valuePropertyKey:
                return object->as<KeyFrameIdBase>()->value();
            case ListenerBoolChangeBase::valuePropertyKey:
                return object->as<ListenerBoolChangeBase>()->value();
            case ListenerAlignTargetBase::targetIdPropertyKey:
                return object->as<ListenerAlignTargetBase>()->targetId();
            case TransitionValueConditionBase::opValuePropertyKey:
                return object->as<TransitionValueConditionBase>()->opValue();
            case StateTransitionBase::stateToIdPropertyKey:
                return object->as<StateTransitionBase>()->stateToId();
            case StateTransitionBase::flagsPropertyKey:
                return object->as<StateTransitionBase>()->flags();
            case StateTransitionBase::durationPropertyKey:
                return object->as<StateTransitionBase>()->duration();
            case StateTransitionBase::exitTimePropertyKey:
                return object->as<StateTransitionBase>()->exitTime();
            case StateTransitionBase::interpolationTypePropertyKey:
                return object->as<StateTransitionBase>()->interpolationType();
            case StateTransitionBase::interpolatorIdPropertyKey:
                return object->as<StateTransitionBase>()->interpolatorId();
            case StateTransitionBase::randomWeightPropertyKey:
                return object->as<StateTransitionBase>()->randomWeight();
            case StateMachineFireEventBase::eventIdPropertyKey:
                return object->as<StateMachineFireEventBase>()->eventId();
            case StateMachineFireEventBase::occursValuePropertyKey:
                return object->as<StateMachineFireEventBase>()->occursValue();
            case LinearAnimationBase::fpsPropertyKey:
                return object->as<LinearAnimationBase>()->fps();
            case LinearAnimationBase::durationPropertyKey:
                return object->as<LinearAnimationBase>()->duration();
            case LinearAnimationBase::loopValuePropertyKey:
                return object->as<LinearAnimationBase>()->loopValue();
            case LinearAnimationBase::workStartPropertyKey:
                return object->as<LinearAnimationBase>()->workStart();
            case LinearAnimationBase::workEndPropertyKey:
                return object->as<LinearAnimationBase>()->workEnd();
            case ElasticInterpolatorBase::easingValuePropertyKey:
                return object->as<ElasticInterpolatorBase>()->easingValue();
            case BlendState1DBase::inputIdPropertyKey:
                return object->as<BlendState1DBase>()->inputId();
            case BlendStateTransitionBase::exitBlendAnimationIdPropertyKey:
                return object->as<BlendStateTransitionBase>()->exitBlendAnimationId();
            case StrokeBase::capPropertyKey:
                return object->as<StrokeBase>()->cap();
            case StrokeBase::joinPropertyKey:
                return object->as<StrokeBase>()->join();
            case TrimPathBase::modeValuePropertyKey:
                return object->as<TrimPathBase>()->modeValue();
            case FillBase::fillRulePropertyKey:
                return object->as<FillBase>()->fillRule();
            case PathBase::pathFlagsPropertyKey:
                return object->as<PathBase>()->pathFlags();
            case ClippingShapeBase::sourceIdPropertyKey:
                return object->as<ClippingShapeBase>()->sourceId();
            case ClippingShapeBase::fillRulePropertyKey:
                return object->as<ClippingShapeBase>()->fillRule();
            case PolygonBase::pointsPropertyKey:
                return object->as<PolygonBase>()->points();
            case ImageBase::assetIdPropertyKey:
                return object->as<ImageBase>()->assetId();
            case DrawRulesBase::drawTargetIdPropertyKey:
                return object->as<DrawRulesBase>()->drawTargetId();
            case LayoutComponentBase::styleIdPropertyKey:
                return object->as<LayoutComponentBase>()->styleId();
            case ArtboardBase::defaultStateMachineIdPropertyKey:
                return object->as<ArtboardBase>()->defaultStateMachineId();
            case ArtboardBase::viewModelIdPropertyKey:
                return object->as<ArtboardBase>()->viewModelId();
            case JoystickBase::xIdPropertyKey:
                return object->as<JoystickBase>()->xId();
            case JoystickBase::yIdPropertyKey:
                return object->as<JoystickBase>()->yId();
            case JoystickBase::joystickFlagsPropertyKey:
                return object->as<JoystickBase>()->joystickFlags();
            case JoystickBase::handleSourceIdPropertyKey:
                return object->as<JoystickBase>()->handleSourceId();
            case OpenUrlEventBase::targetValuePropertyKey:
                return object->as<OpenUrlEventBase>()->targetValue();
            case DataBindBase::targetIdPropertyKey:
                return object->as<DataBindBase>()->targetId();
            case DataBindBase::propertyKeyPropertyKey:
                return object->as<DataBindBase>()->propertyKey();
            case DataBindBase::modeValuePropertyKey:
                return object->as<DataBindBase>()->modeValue();
            case WeightBase::valuesPropertyKey:
                return object->as<WeightBase>()->values();
            case WeightBase::indicesPropertyKey:
                return object->as<WeightBase>()->indices();
            case TendonBase::boneIdPropertyKey:
                return object->as<TendonBase>()->boneId();
            case CubicWeightBase::inValuesPropertyKey:
                return object->as<CubicWeightBase>()->inValues();
            case CubicWeightBase::inIndicesPropertyKey:
                return object->as<CubicWeightBase>()->inIndices();
            case CubicWeightBase::outValuesPropertyKey:
                return object->as<CubicWeightBase>()->outValues();
            case CubicWeightBase::outIndicesPropertyKey:
                return object->as<CubicWeightBase>()->outIndices();
            case TextModifierRangeBase::unitsValuePropertyKey:
                return object->as<TextModifierRangeBase>()->unitsValue();
            case TextModifierRangeBase::typeValuePropertyKey:
                return object->as<TextModifierRangeBase>()->typeValue();
            case TextModifierRangeBase::modeValuePropertyKey:
                return object->as<TextModifierRangeBase>()->modeValue();
            case TextModifierRangeBase::runIdPropertyKey:
                return object->as<TextModifierRangeBase>()->runId();
            case TextStyleFeatureBase::tagPropertyKey:
                return object->as<TextStyleFeatureBase>()->tag();
            case TextStyleFeatureBase::featureValuePropertyKey:
                return object->as<TextStyleFeatureBase>()->featureValue();
            case TextVariationModifierBase::axisTagPropertyKey:
                return object->as<TextVariationModifierBase>()->axisTag();
            case TextModifierGroupBase::modifierFlagsPropertyKey:
                return object->as<TextModifierGroupBase>()->modifierFlags();
            case TextStyleBase::fontAssetIdPropertyKey:
                return object->as<TextStyleBase>()->fontAssetId();
            case TextStyleAxisBase::tagPropertyKey:
                return object->as<TextStyleAxisBase>()->tag();
            case TextBase::alignValuePropertyKey:
                return object->as<TextBase>()->alignValue();
            case TextBase::sizingValuePropertyKey:
                return object->as<TextBase>()->sizingValue();
            case TextBase::overflowValuePropertyKey:
                return object->as<TextBase>()->overflowValue();
            case TextBase::originValuePropertyKey:
                return object->as<TextBase>()->originValue();
            case TextValueRunBase::styleIdPropertyKey:
                return object->as<TextValueRunBase>()->styleId();
            case FileAssetBase::assetIdPropertyKey:
                return object->as<FileAssetBase>()->assetId();
            case AudioEventBase::assetIdPropertyKey:
                return object->as<AudioEventBase>()->assetId();
        }
        return 0;
    }
    static int getColor(Core* object, int propertyKey)
    {
        switch (propertyKey)
        {
            case ViewModelInstanceColorBase::propertyValuePropertyKey:
                return object->as<ViewModelInstanceColorBase>()->propertyValue();
            case KeyFrameColorBase::valuePropertyKey:
                return object->as<KeyFrameColorBase>()->value();
            case SolidColorBase::colorValuePropertyKey:
                return object->as<SolidColorBase>()->colorValue();
            case GradientStopBase::colorValuePropertyKey:
                return object->as<GradientStopBase>()->colorValue();
        }
        return 0;
    }
    static std::string getString(Core* object, int propertyKey)
    {
        switch (propertyKey)
        {
            case ViewModelComponentBase::namePropertyKey:
                return object->as<ViewModelComponentBase>()->name();
            case ViewModelInstanceStringBase::propertyValuePropertyKey:
                return object->as<ViewModelInstanceStringBase>()->propertyValue();
            case ComponentBase::namePropertyKey:
                return object->as<ComponentBase>()->name();
            case DataEnumValueBase::keyPropertyKey:
                return object->as<DataEnumValueBase>()->key();
            case DataEnumValueBase::valuePropertyKey:
                return object->as<DataEnumValueBase>()->value();
            case AnimationBase::namePropertyKey:
                return object->as<AnimationBase>()->name();
            case StateMachineComponentBase::namePropertyKey:
                return object->as<StateMachineComponentBase>()->name();
            case KeyFrameStringBase::valuePropertyKey:
                return object->as<KeyFrameStringBase>()->value();
            case OpenUrlEventBase::urlPropertyKey:
                return object->as<OpenUrlEventBase>()->url();
            case TextValueRunBase::textPropertyKey:
                return object->as<TextValueRunBase>()->text();
            case CustomPropertyStringBase::propertyValuePropertyKey:
                return object->as<CustomPropertyStringBase>()->propertyValue();
            case AssetBase::namePropertyKey:
                return object->as<AssetBase>()->name();
            case FileAssetBase::cdnBaseUrlPropertyKey:
                return object->as<FileAssetBase>()->cdnBaseUrl();
        }
        return "";
    }
    static float getDouble(Core* object, int propertyKey)
    {
        switch (propertyKey)
        {
            case ViewModelInstanceNumberBase::propertyValuePropertyKey:
                return object->as<ViewModelInstanceNumberBase>()->propertyValue();
            case CustomPropertyNumberBase::propertyValuePropertyKey:
                return object->as<CustomPropertyNumberBase>()->propertyValue();
            case ConstraintBase::strengthPropertyKey:
                return object->as<ConstraintBase>()->strength();
            case DistanceConstraintBase::distancePropertyKey:
                return object->as<DistanceConstraintBase>()->distance();
            case TransformComponentConstraintBase::copyFactorPropertyKey:
                return object->as<TransformComponentConstraintBase>()->copyFactor();
            case TransformComponentConstraintBase::minValuePropertyKey:
                return object->as<TransformComponentConstraintBase>()->minValue();
            case TransformComponentConstraintBase::maxValuePropertyKey:
                return object->as<TransformComponentConstraintBase>()->maxValue();
            case TransformComponentConstraintYBase::copyFactorYPropertyKey:
                return object->as<TransformComponentConstraintYBase>()->copyFactorY();
            case TransformComponentConstraintYBase::minValueYPropertyKey:
                return object->as<TransformComponentConstraintYBase>()->minValueY();
            case TransformComponentConstraintYBase::maxValueYPropertyKey:
                return object->as<TransformComponentConstraintYBase>()->maxValueY();
            case FollowPathConstraintBase::distancePropertyKey:
                return object->as<FollowPathConstraintBase>()->distance();
            case TransformConstraintBase::originXPropertyKey:
                return object->as<TransformConstraintBase>()->originX();
            case TransformConstraintBase::originYPropertyKey:
                return object->as<TransformConstraintBase>()->originY();
            case WorldTransformComponentBase::opacityPropertyKey:
                return object->as<WorldTransformComponentBase>()->opacity();
            case TransformComponentBase::rotationPropertyKey:
                return object->as<TransformComponentBase>()->rotation();
            case TransformComponentBase::scaleXPropertyKey:
                return object->as<TransformComponentBase>()->scaleX();
            case TransformComponentBase::scaleYPropertyKey:
                return object->as<TransformComponentBase>()->scaleY();
            case NodeBase::xPropertyKey:
                return object->as<NodeBase>()->x();
            case NodeBase::yPropertyKey:
                return object->as<NodeBase>()->y();
            case LayoutComponentStyleBase::gapHorizontalPropertyKey:
                return object->as<LayoutComponentStyleBase>()->gapHorizontal();
            case LayoutComponentStyleBase::gapVerticalPropertyKey:
                return object->as<LayoutComponentStyleBase>()->gapVertical();
            case LayoutComponentStyleBase::maxWidthPropertyKey:
                return object->as<LayoutComponentStyleBase>()->maxWidth();
            case LayoutComponentStyleBase::maxHeightPropertyKey:
                return object->as<LayoutComponentStyleBase>()->maxHeight();
            case LayoutComponentStyleBase::minWidthPropertyKey:
                return object->as<LayoutComponentStyleBase>()->minWidth();
            case LayoutComponentStyleBase::minHeightPropertyKey:
                return object->as<LayoutComponentStyleBase>()->minHeight();
            case LayoutComponentStyleBase::borderLeftPropertyKey:
                return object->as<LayoutComponentStyleBase>()->borderLeft();
            case LayoutComponentStyleBase::borderRightPropertyKey:
                return object->as<LayoutComponentStyleBase>()->borderRight();
            case LayoutComponentStyleBase::borderTopPropertyKey:
                return object->as<LayoutComponentStyleBase>()->borderTop();
            case LayoutComponentStyleBase::borderBottomPropertyKey:
                return object->as<LayoutComponentStyleBase>()->borderBottom();
            case LayoutComponentStyleBase::marginLeftPropertyKey:
                return object->as<LayoutComponentStyleBase>()->marginLeft();
            case LayoutComponentStyleBase::marginRightPropertyKey:
                return object->as<LayoutComponentStyleBase>()->marginRight();
            case LayoutComponentStyleBase::marginTopPropertyKey:
                return object->as<LayoutComponentStyleBase>()->marginTop();
            case LayoutComponentStyleBase::marginBottomPropertyKey:
                return object->as<LayoutComponentStyleBase>()->marginBottom();
            case LayoutComponentStyleBase::paddingLeftPropertyKey:
                return object->as<LayoutComponentStyleBase>()->paddingLeft();
            case LayoutComponentStyleBase::paddingRightPropertyKey:
                return object->as<LayoutComponentStyleBase>()->paddingRight();
            case LayoutComponentStyleBase::paddingTopPropertyKey:
                return object->as<LayoutComponentStyleBase>()->paddingTop();
            case LayoutComponentStyleBase::paddingBottomPropertyKey:
                return object->as<LayoutComponentStyleBase>()->paddingBottom();
            case LayoutComponentStyleBase::positionLeftPropertyKey:
                return object->as<LayoutComponentStyleBase>()->positionLeft();
            case LayoutComponentStyleBase::positionRightPropertyKey:
                return object->as<LayoutComponentStyleBase>()->positionRight();
            case LayoutComponentStyleBase::positionTopPropertyKey:
                return object->as<LayoutComponentStyleBase>()->positionTop();
            case LayoutComponentStyleBase::positionBottomPropertyKey:
                return object->as<LayoutComponentStyleBase>()->positionBottom();
            case LayoutComponentStyleBase::flexPropertyKey:
                return object->as<LayoutComponentStyleBase>()->flex();
            case LayoutComponentStyleBase::flexGrowPropertyKey:
                return object->as<LayoutComponentStyleBase>()->flexGrow();
            case LayoutComponentStyleBase::flexShrinkPropertyKey:
                return object->as<LayoutComponentStyleBase>()->flexShrink();
            case LayoutComponentStyleBase::flexBasisPropertyKey:
                return object->as<LayoutComponentStyleBase>()->flexBasis();
            case LayoutComponentStyleBase::aspectRatioPropertyKey:
                return object->as<LayoutComponentStyleBase>()->aspectRatio();
            case NestedLinearAnimationBase::mixPropertyKey:
                return object->as<NestedLinearAnimationBase>()->mix();
            case NestedSimpleAnimationBase::speedPropertyKey:
                return object->as<NestedSimpleAnimationBase>()->speed();
            case AdvanceableStateBase::speedPropertyKey:
                return object->as<AdvanceableStateBase>()->speed();
            case BlendAnimationDirectBase::mixValuePropertyKey:
                return object->as<BlendAnimationDirectBase>()->mixValue();
            case StateMachineNumberBase::valuePropertyKey:
                return object->as<StateMachineNumberBase>()->value();
            case CubicInterpolatorBase::x1PropertyKey:
                return object->as<CubicInterpolatorBase>()->x1();
            case CubicInterpolatorBase::y1PropertyKey:
                return object->as<CubicInterpolatorBase>()->y1();
            case CubicInterpolatorBase::x2PropertyKey:
                return object->as<CubicInterpolatorBase>()->x2();
            case CubicInterpolatorBase::y2PropertyKey:
                return object->as<CubicInterpolatorBase>()->y2();
            case TransitionNumberConditionBase::valuePropertyKey:
                return object->as<TransitionNumberConditionBase>()->value();
            case CubicInterpolatorComponentBase::x1PropertyKey:
                return object->as<CubicInterpolatorComponentBase>()->x1();
            case CubicInterpolatorComponentBase::y1PropertyKey:
                return object->as<CubicInterpolatorComponentBase>()->y1();
            case CubicInterpolatorComponentBase::x2PropertyKey:
                return object->as<CubicInterpolatorComponentBase>()->x2();
            case CubicInterpolatorComponentBase::y2PropertyKey:
                return object->as<CubicInterpolatorComponentBase>()->y2();
            case ListenerNumberChangeBase::valuePropertyKey:
                return object->as<ListenerNumberChangeBase>()->value();
            case KeyFrameDoubleBase::valuePropertyKey:
                return object->as<KeyFrameDoubleBase>()->value();
            case LinearAnimationBase::speedPropertyKey:
                return object->as<LinearAnimationBase>()->speed();
            case ElasticInterpolatorBase::amplitudePropertyKey:
                return object->as<ElasticInterpolatorBase>()->amplitude();
            case ElasticInterpolatorBase::periodPropertyKey:
                return object->as<ElasticInterpolatorBase>()->period();
            case NestedNumberBase::nestedValuePropertyKey:
                return object->as<NestedNumberBase>()->nestedValue();
            case NestedRemapAnimationBase::timePropertyKey:
                return object->as<NestedRemapAnimationBase>()->time();
            case BlendAnimation1DBase::valuePropertyKey:
                return object->as<BlendAnimation1DBase>()->value();
            case LinearGradientBase::startXPropertyKey:
                return object->as<LinearGradientBase>()->startX();
            case LinearGradientBase::startYPropertyKey:
                return object->as<LinearGradientBase>()->startY();
            case LinearGradientBase::endXPropertyKey:
                return object->as<LinearGradientBase>()->endX();
            case LinearGradientBase::endYPropertyKey:
                return object->as<LinearGradientBase>()->endY();
            case LinearGradientBase::opacityPropertyKey:
                return object->as<LinearGradientBase>()->opacity();
            case StrokeBase::thicknessPropertyKey:
                return object->as<StrokeBase>()->thickness();
            case GradientStopBase::positionPropertyKey:
                return object->as<GradientStopBase>()->position();
            case TrimPathBase::startPropertyKey:
                return object->as<TrimPathBase>()->start();
            case TrimPathBase::endPropertyKey:
                return object->as<TrimPathBase>()->end();
            case TrimPathBase::offsetPropertyKey:
                return object->as<TrimPathBase>()->offset();
            case VertexBase::xPropertyKey:
                return object->as<VertexBase>()->x();
            case VertexBase::yPropertyKey:
                return object->as<VertexBase>()->y();
            case MeshVertexBase::uPropertyKey:
                return object->as<MeshVertexBase>()->u();
            case MeshVertexBase::vPropertyKey:
                return object->as<MeshVertexBase>()->v();
            case StraightVertexBase::radiusPropertyKey:
                return object->as<StraightVertexBase>()->radius();
            case CubicAsymmetricVertexBase::rotationPropertyKey:
                return object->as<CubicAsymmetricVertexBase>()->rotation();
            case CubicAsymmetricVertexBase::inDistancePropertyKey:
                return object->as<CubicAsymmetricVertexBase>()->inDistance();
            case CubicAsymmetricVertexBase::outDistancePropertyKey:
                return object->as<CubicAsymmetricVertexBase>()->outDistance();
            case ParametricPathBase::widthPropertyKey:
                return object->as<ParametricPathBase>()->width();
            case ParametricPathBase::heightPropertyKey:
                return object->as<ParametricPathBase>()->height();
            case ParametricPathBase::originXPropertyKey:
                return object->as<ParametricPathBase>()->originX();
            case ParametricPathBase::originYPropertyKey:
                return object->as<ParametricPathBase>()->originY();
            case RectangleBase::cornerRadiusTLPropertyKey:
                return object->as<RectangleBase>()->cornerRadiusTL();
            case RectangleBase::cornerRadiusTRPropertyKey:
                return object->as<RectangleBase>()->cornerRadiusTR();
            case RectangleBase::cornerRadiusBLPropertyKey:
                return object->as<RectangleBase>()->cornerRadiusBL();
            case RectangleBase::cornerRadiusBRPropertyKey:
                return object->as<RectangleBase>()->cornerRadiusBR();
            case CubicMirroredVertexBase::rotationPropertyKey:
                return object->as<CubicMirroredVertexBase>()->rotation();
            case CubicMirroredVertexBase::distancePropertyKey:
                return object->as<CubicMirroredVertexBase>()->distance();
            case PolygonBase::cornerRadiusPropertyKey:
                return object->as<PolygonBase>()->cornerRadius();
            case StarBase::innerRadiusPropertyKey:
                return object->as<StarBase>()->innerRadius();
            case ImageBase::originXPropertyKey:
                return object->as<ImageBase>()->originX();
            case ImageBase::originYPropertyKey:
                return object->as<ImageBase>()->originY();
            case CubicDetachedVertexBase::inRotationPropertyKey:
                return object->as<CubicDetachedVertexBase>()->inRotation();
            case CubicDetachedVertexBase::inDistancePropertyKey:
                return object->as<CubicDetachedVertexBase>()->inDistance();
            case CubicDetachedVertexBase::outRotationPropertyKey:
                return object->as<CubicDetachedVertexBase>()->outRotation();
            case CubicDetachedVertexBase::outDistancePropertyKey:
                return object->as<CubicDetachedVertexBase>()->outDistance();
            case LayoutComponentBase::widthPropertyKey:
                return object->as<LayoutComponentBase>()->width();
            case LayoutComponentBase::heightPropertyKey:
                return object->as<LayoutComponentBase>()->height();
            case ArtboardBase::xPropertyKey:
                return object->as<ArtboardBase>()->x();
            case ArtboardBase::yPropertyKey:
                return object->as<ArtboardBase>()->y();
            case ArtboardBase::originXPropertyKey:
                return object->as<ArtboardBase>()->originX();
            case ArtboardBase::originYPropertyKey:
                return object->as<ArtboardBase>()->originY();
            case JoystickBase::xPropertyKey:
                return object->as<JoystickBase>()->x();
            case JoystickBase::yPropertyKey:
                return object->as<JoystickBase>()->y();
            case JoystickBase::posXPropertyKey:
                return object->as<JoystickBase>()->posX();
            case JoystickBase::posYPropertyKey:
                return object->as<JoystickBase>()->posY();
            case JoystickBase::originXPropertyKey:
                return object->as<JoystickBase>()->originX();
            case JoystickBase::originYPropertyKey:
                return object->as<JoystickBase>()->originY();
            case JoystickBase::widthPropertyKey:
                return object->as<JoystickBase>()->width();
            case JoystickBase::heightPropertyKey:
                return object->as<JoystickBase>()->height();
            case BoneBase::lengthPropertyKey:
                return object->as<BoneBase>()->length();
            case RootBoneBase::xPropertyKey:
                return object->as<RootBoneBase>()->x();
            case RootBoneBase::yPropertyKey:
                return object->as<RootBoneBase>()->y();
            case SkinBase::xxPropertyKey:
                return object->as<SkinBase>()->xx();
            case SkinBase::yxPropertyKey:
                return object->as<SkinBase>()->yx();
            case SkinBase::xyPropertyKey:
                return object->as<SkinBase>()->xy();
            case SkinBase::yyPropertyKey:
                return object->as<SkinBase>()->yy();
            case SkinBase::txPropertyKey:
                return object->as<SkinBase>()->tx();
            case SkinBase::tyPropertyKey:
                return object->as<SkinBase>()->ty();
            case TendonBase::xxPropertyKey:
                return object->as<TendonBase>()->xx();
            case TendonBase::yxPropertyKey:
                return object->as<TendonBase>()->yx();
            case TendonBase::xyPropertyKey:
                return object->as<TendonBase>()->xy();
            case TendonBase::yyPropertyKey:
                return object->as<TendonBase>()->yy();
            case TendonBase::txPropertyKey:
                return object->as<TendonBase>()->tx();
            case TendonBase::tyPropertyKey:
                return object->as<TendonBase>()->ty();
            case TextModifierRangeBase::modifyFromPropertyKey:
                return object->as<TextModifierRangeBase>()->modifyFrom();
            case TextModifierRangeBase::modifyToPropertyKey:
                return object->as<TextModifierRangeBase>()->modifyTo();
            case TextModifierRangeBase::strengthPropertyKey:
                return object->as<TextModifierRangeBase>()->strength();
            case TextModifierRangeBase::falloffFromPropertyKey:
                return object->as<TextModifierRangeBase>()->falloffFrom();
            case TextModifierRangeBase::falloffToPropertyKey:
                return object->as<TextModifierRangeBase>()->falloffTo();
            case TextModifierRangeBase::offsetPropertyKey:
                return object->as<TextModifierRangeBase>()->offset();
            case TextVariationModifierBase::axisValuePropertyKey:
                return object->as<TextVariationModifierBase>()->axisValue();
            case TextModifierGroupBase::originXPropertyKey:
                return object->as<TextModifierGroupBase>()->originX();
            case TextModifierGroupBase::originYPropertyKey:
                return object->as<TextModifierGroupBase>()->originY();
            case TextModifierGroupBase::opacityPropertyKey:
                return object->as<TextModifierGroupBase>()->opacity();
            case TextModifierGroupBase::xPropertyKey:
                return object->as<TextModifierGroupBase>()->x();
            case TextModifierGroupBase::yPropertyKey:
                return object->as<TextModifierGroupBase>()->y();
            case TextModifierGroupBase::rotationPropertyKey:
                return object->as<TextModifierGroupBase>()->rotation();
            case TextModifierGroupBase::scaleXPropertyKey:
                return object->as<TextModifierGroupBase>()->scaleX();
            case TextModifierGroupBase::scaleYPropertyKey:
                return object->as<TextModifierGroupBase>()->scaleY();
            case TextStyleBase::fontSizePropertyKey:
                return object->as<TextStyleBase>()->fontSize();
            case TextStyleBase::lineHeightPropertyKey:
                return object->as<TextStyleBase>()->lineHeight();
            case TextStyleBase::letterSpacingPropertyKey:
                return object->as<TextStyleBase>()->letterSpacing();
            case TextStyleAxisBase::axisValuePropertyKey:
                return object->as<TextStyleAxisBase>()->axisValue();
            case TextBase::widthPropertyKey:
                return object->as<TextBase>()->width();
            case TextBase::heightPropertyKey:
                return object->as<TextBase>()->height();
            case TextBase::originXPropertyKey:
                return object->as<TextBase>()->originX();
            case TextBase::originYPropertyKey:
                return object->as<TextBase>()->originY();
            case TextBase::paragraphSpacingPropertyKey:
                return object->as<TextBase>()->paragraphSpacing();
            case DrawableAssetBase::heightPropertyKey:
                return object->as<DrawableAssetBase>()->height();
            case DrawableAssetBase::widthPropertyKey:
                return object->as<DrawableAssetBase>()->width();
            case ExportAudioBase::volumePropertyKey:
                return object->as<ExportAudioBase>()->volume();
        }
        return 0.0f;
    }
    static int propertyFieldId(int propertyKey)
    {
        switch (propertyKey)
        {
            case ViewModelInstanceListItemBase::useLinkedArtboardPropertyKey:
            case TransformComponentConstraintBase::offsetPropertyKey:
            case TransformComponentConstraintBase::doesCopyPropertyKey:
            case TransformComponentConstraintBase::minPropertyKey:
            case TransformComponentConstraintBase::maxPropertyKey:
            case TransformComponentConstraintYBase::doesCopyYPropertyKey:
            case TransformComponentConstraintYBase::minYPropertyKey:
            case TransformComponentConstraintYBase::maxYPropertyKey:
            case IKConstraintBase::invertDirectionPropertyKey:
            case FollowPathConstraintBase::orientPropertyKey:
            case FollowPathConstraintBase::offsetPropertyKey:
            case NestedSimpleAnimationBase::isPlayingPropertyKey:
            case KeyFrameBoolBase::valuePropertyKey:
            case ListenerAlignTargetBase::preserveOffsetPropertyKey:
            case NestedBoolBase::nestedValuePropertyKey:
            case LinearAnimationBase::enableWorkAreaPropertyKey:
            case LinearAnimationBase::quantizePropertyKey:
            case StateMachineBoolBase::valuePropertyKey:
            case ShapePaintBase::isVisiblePropertyKey:
            case StrokeBase::transformAffectsStrokePropertyKey:
            case PointsPathBase::isClosedPropertyKey:
            case RectangleBase::linkCornerRadiusPropertyKey:
            case ClippingShapeBase::isVisiblePropertyKey:
            case CustomPropertyBooleanBase::propertyValuePropertyKey:
            case LayoutComponentBase::clipPropertyKey:
            case TextModifierRangeBase::clampPropertyKey:
                return CoreBoolType::id;
            case ViewModelInstanceListItemBase::viewModelIdPropertyKey:
            case ViewModelInstanceListItemBase::viewModelInstanceIdPropertyKey:
            case ViewModelInstanceListItemBase::artboardIdPropertyKey:
            case ViewModelInstanceValueBase::viewModelPropertyIdPropertyKey:
            case ViewModelInstanceEnumBase::propertyValuePropertyKey:
            case ViewModelBase::defaultInstanceIdPropertyKey:
            case ViewModelPropertyViewModelBase::viewModelReferenceIdPropertyKey:
            case ComponentBase::parentIdPropertyKey:
            case ViewModelInstanceBase::viewModelIdPropertyKey:
            case ViewModelPropertyEnumBase::enumIdPropertyKey:
            case ViewModelInstanceViewModelBase::propertyValuePropertyKey:
            case DrawTargetBase::drawableIdPropertyKey:
            case DrawTargetBase::placementValuePropertyKey:
            case TargetedConstraintBase::targetIdPropertyKey:
            case DistanceConstraintBase::modeValuePropertyKey:
            case TransformSpaceConstraintBase::sourceSpaceValuePropertyKey:
            case TransformSpaceConstraintBase::destSpaceValuePropertyKey:
            case TransformComponentConstraintBase::minMaxSpaceValuePropertyKey:
            case IKConstraintBase::parentBoneCountPropertyKey:
            case DrawableBase::blendModeValuePropertyKey:
            case DrawableBase::drawableFlagsPropertyKey:
            case NestedArtboardBase::artboardIdPropertyKey:
            case NestedArtboardBase::fitPropertyKey:
            case NestedArtboardBase::alignmentPropertyKey:
            case NestedAnimationBase::animationIdPropertyKey:
            case SoloBase::activeComponentIdPropertyKey:
            case LayoutComponentStyleBase::layoutFlags0PropertyKey:
            case LayoutComponentStyleBase::layoutFlags1PropertyKey:
            case LayoutComponentStyleBase::layoutFlags2PropertyKey:
            case ListenerFireEventBase::eventIdPropertyKey:
            case LayerStateBase::flagsPropertyKey:
            case ListenerInputChangeBase::inputIdPropertyKey:
            case ListenerInputChangeBase::nestedInputIdPropertyKey:
            case AnimationStateBase::animationIdPropertyKey:
            case NestedInputBase::inputIdPropertyKey:
            case KeyedObjectBase::objectIdPropertyKey:
            case BlendAnimationBase::animationIdPropertyKey:
            case BlendAnimationDirectBase::inputIdPropertyKey:
            case BlendAnimationDirectBase::blendSourcePropertyKey:
            case TransitionConditionBase::inputIdPropertyKey:
            case KeyedPropertyBase::propertyKeyPropertyKey:
            case StateMachineListenerBase::targetIdPropertyKey:
            case StateMachineListenerBase::listenerTypeValuePropertyKey:
            case StateMachineListenerBase::eventIdPropertyKey:
            case KeyFrameBase::framePropertyKey:
            case InterpolatingKeyFrameBase::interpolationTypePropertyKey:
            case InterpolatingKeyFrameBase::interpolatorIdPropertyKey:
            case KeyFrameIdBase::valuePropertyKey:
            case ListenerBoolChangeBase::valuePropertyKey:
            case ListenerAlignTargetBase::targetIdPropertyKey:
            case TransitionValueConditionBase::opValuePropertyKey:
            case StateTransitionBase::stateToIdPropertyKey:
            case StateTransitionBase::flagsPropertyKey:
            case StateTransitionBase::durationPropertyKey:
            case StateTransitionBase::exitTimePropertyKey:
            case StateTransitionBase::interpolationTypePropertyKey:
            case StateTransitionBase::interpolatorIdPropertyKey:
            case StateTransitionBase::randomWeightPropertyKey:
            case StateMachineFireEventBase::eventIdPropertyKey:
            case StateMachineFireEventBase::occursValuePropertyKey:
            case LinearAnimationBase::fpsPropertyKey:
            case LinearAnimationBase::durationPropertyKey:
            case LinearAnimationBase::loopValuePropertyKey:
            case LinearAnimationBase::workStartPropertyKey:
            case LinearAnimationBase::workEndPropertyKey:
            case ElasticInterpolatorBase::easingValuePropertyKey:
            case BlendState1DBase::inputIdPropertyKey:
            case BlendStateTransitionBase::exitBlendAnimationIdPropertyKey:
            case StrokeBase::capPropertyKey:
            case StrokeBase::joinPropertyKey:
            case TrimPathBase::modeValuePropertyKey:
            case FillBase::fillRulePropertyKey:
            case PathBase::pathFlagsPropertyKey:
            case ClippingShapeBase::sourceIdPropertyKey:
            case ClippingShapeBase::fillRulePropertyKey:
            case PolygonBase::pointsPropertyKey:
            case ImageBase::assetIdPropertyKey:
            case DrawRulesBase::drawTargetIdPropertyKey:
            case LayoutComponentBase::styleIdPropertyKey:
            case ArtboardBase::defaultStateMachineIdPropertyKey:
            case ArtboardBase::viewModelIdPropertyKey:
            case JoystickBase::xIdPropertyKey:
            case JoystickBase::yIdPropertyKey:
            case JoystickBase::joystickFlagsPropertyKey:
            case JoystickBase::handleSourceIdPropertyKey:
            case OpenUrlEventBase::targetValuePropertyKey:
            case DataBindBase::targetIdPropertyKey:
            case DataBindBase::propertyKeyPropertyKey:
            case DataBindBase::modeValuePropertyKey:
            case WeightBase::valuesPropertyKey:
            case WeightBase::indicesPropertyKey:
            case TendonBase::boneIdPropertyKey:
            case CubicWeightBase::inValuesPropertyKey:
            case CubicWeightBase::inIndicesPropertyKey:
            case CubicWeightBase::outValuesPropertyKey:
            case CubicWeightBase::outIndicesPropertyKey:
            case TextModifierRangeBase::unitsValuePropertyKey:
            case TextModifierRangeBase::typeValuePropertyKey:
            case TextModifierRangeBase::modeValuePropertyKey:
            case TextModifierRangeBase::runIdPropertyKey:
            case TextStyleFeatureBase::tagPropertyKey:
            case TextStyleFeatureBase::featureValuePropertyKey:
            case TextVariationModifierBase::axisTagPropertyKey:
            case TextModifierGroupBase::modifierFlagsPropertyKey:
            case TextStyleBase::fontAssetIdPropertyKey:
            case TextStyleAxisBase::tagPropertyKey:
            case TextBase::alignValuePropertyKey:
            case TextBase::sizingValuePropertyKey:
            case TextBase::overflowValuePropertyKey:
            case TextBase::originValuePropertyKey:
            case TextValueRunBase::styleIdPropertyKey:
            case FileAssetBase::assetIdPropertyKey:
            case AudioEventBase::assetIdPropertyKey:
                return CoreUintType::id;
            case ViewModelInstanceColorBase::propertyValuePropertyKey:
            case KeyFrameColorBase::valuePropertyKey:
            case SolidColorBase::colorValuePropertyKey:
            case GradientStopBase::colorValuePropertyKey:
                return CoreColorType::id;
            case ViewModelComponentBase::namePropertyKey:
            case ViewModelInstanceStringBase::propertyValuePropertyKey:
            case ComponentBase::namePropertyKey:
            case DataEnumValueBase::keyPropertyKey:
            case DataEnumValueBase::valuePropertyKey:
            case AnimationBase::namePropertyKey:
            case StateMachineComponentBase::namePropertyKey:
            case KeyFrameStringBase::valuePropertyKey:
            case OpenUrlEventBase::urlPropertyKey:
            case TextValueRunBase::textPropertyKey:
            case CustomPropertyStringBase::propertyValuePropertyKey:
            case AssetBase::namePropertyKey:
            case FileAssetBase::cdnBaseUrlPropertyKey:
                return CoreStringType::id;
            case ViewModelInstanceNumberBase::propertyValuePropertyKey:
            case CustomPropertyNumberBase::propertyValuePropertyKey:
            case ConstraintBase::strengthPropertyKey:
            case DistanceConstraintBase::distancePropertyKey:
            case TransformComponentConstraintBase::copyFactorPropertyKey:
            case TransformComponentConstraintBase::minValuePropertyKey:
            case TransformComponentConstraintBase::maxValuePropertyKey:
            case TransformComponentConstraintYBase::copyFactorYPropertyKey:
            case TransformComponentConstraintYBase::minValueYPropertyKey:
            case TransformComponentConstraintYBase::maxValueYPropertyKey:
            case FollowPathConstraintBase::distancePropertyKey:
            case TransformConstraintBase::originXPropertyKey:
            case TransformConstraintBase::originYPropertyKey:
            case WorldTransformComponentBase::opacityPropertyKey:
            case TransformComponentBase::rotationPropertyKey:
            case TransformComponentBase::scaleXPropertyKey:
            case TransformComponentBase::scaleYPropertyKey:
            case NodeBase::xPropertyKey:
            case NodeBase::yPropertyKey:
            case LayoutComponentStyleBase::gapHorizontalPropertyKey:
            case LayoutComponentStyleBase::gapVerticalPropertyKey:
            case LayoutComponentStyleBase::maxWidthPropertyKey:
            case LayoutComponentStyleBase::maxHeightPropertyKey:
            case LayoutComponentStyleBase::minWidthPropertyKey:
            case LayoutComponentStyleBase::minHeightPropertyKey:
            case LayoutComponentStyleBase::borderLeftPropertyKey:
            case LayoutComponentStyleBase::borderRightPropertyKey:
            case LayoutComponentStyleBase::borderTopPropertyKey:
            case LayoutComponentStyleBase::borderBottomPropertyKey:
            case LayoutComponentStyleBase::marginLeftPropertyKey:
            case LayoutComponentStyleBase::marginRightPropertyKey:
            case LayoutComponentStyleBase::marginTopPropertyKey:
            case LayoutComponentStyleBase::marginBottomPropertyKey:
            case LayoutComponentStyleBase::paddingLeftPropertyKey:
            case LayoutComponentStyleBase::paddingRightPropertyKey:
            case LayoutComponentStyleBase::paddingTopPropertyKey:
            case LayoutComponentStyleBase::paddingBottomPropertyKey:
            case LayoutComponentStyleBase::positionLeftPropertyKey:
            case LayoutComponentStyleBase::positionRightPropertyKey:
            case LayoutComponentStyleBase::positionTopPropertyKey:
            case LayoutComponentStyleBase::positionBottomPropertyKey:
            case LayoutComponentStyleBase::flexPropertyKey:
            case LayoutComponentStyleBase::flexGrowPropertyKey:
            case LayoutComponentStyleBase::flexShrinkPropertyKey:
            case LayoutComponentStyleBase::flexBasisPropertyKey:
            case LayoutComponentStyleBase::aspectRatioPropertyKey:
            case NestedLinearAnimationBase::mixPropertyKey:
            case NestedSimpleAnimationBase::speedPropertyKey:
            case AdvanceableStateBase::speedPropertyKey:
            case BlendAnimationDirectBase::mixValuePropertyKey:
            case StateMachineNumberBase::valuePropertyKey:
            case CubicInterpolatorBase::x1PropertyKey:
            case CubicInterpolatorBase::y1PropertyKey:
            case CubicInterpolatorBase::x2PropertyKey:
            case CubicInterpolatorBase::y2PropertyKey:
            case TransitionNumberConditionBase::valuePropertyKey:
            case CubicInterpolatorComponentBase::x1PropertyKey:
            case CubicInterpolatorComponentBase::y1PropertyKey:
            case CubicInterpolatorComponentBase::x2PropertyKey:
            case CubicInterpolatorComponentBase::y2PropertyKey:
            case ListenerNumberChangeBase::valuePropertyKey:
            case KeyFrameDoubleBase::valuePropertyKey:
            case LinearAnimationBase::speedPropertyKey:
            case ElasticInterpolatorBase::amplitudePropertyKey:
            case ElasticInterpolatorBase::periodPropertyKey:
            case NestedNumberBase::nestedValuePropertyKey:
            case NestedRemapAnimationBase::timePropertyKey:
            case BlendAnimation1DBase::valuePropertyKey:
            case LinearGradientBase::startXPropertyKey:
            case LinearGradientBase::startYPropertyKey:
            case LinearGradientBase::endXPropertyKey:
            case LinearGradientBase::endYPropertyKey:
            case LinearGradientBase::opacityPropertyKey:
            case StrokeBase::thicknessPropertyKey:
            case GradientStopBase::positionPropertyKey:
            case TrimPathBase::startPropertyKey:
            case TrimPathBase::endPropertyKey:
            case TrimPathBase::offsetPropertyKey:
            case VertexBase::xPropertyKey:
            case VertexBase::yPropertyKey:
            case MeshVertexBase::uPropertyKey:
            case MeshVertexBase::vPropertyKey:
            case StraightVertexBase::radiusPropertyKey:
            case CubicAsymmetricVertexBase::rotationPropertyKey:
            case CubicAsymmetricVertexBase::inDistancePropertyKey:
            case CubicAsymmetricVertexBase::outDistancePropertyKey:
            case ParametricPathBase::widthPropertyKey:
            case ParametricPathBase::heightPropertyKey:
            case ParametricPathBase::originXPropertyKey:
            case ParametricPathBase::originYPropertyKey:
            case RectangleBase::cornerRadiusTLPropertyKey:
            case RectangleBase::cornerRadiusTRPropertyKey:
            case RectangleBase::cornerRadiusBLPropertyKey:
            case RectangleBase::cornerRadiusBRPropertyKey:
            case CubicMirroredVertexBase::rotationPropertyKey:
            case CubicMirroredVertexBase::distancePropertyKey:
            case PolygonBase::cornerRadiusPropertyKey:
            case StarBase::innerRadiusPropertyKey:
            case ImageBase::originXPropertyKey:
            case ImageBase::originYPropertyKey:
            case CubicDetachedVertexBase::inRotationPropertyKey:
            case CubicDetachedVertexBase::inDistancePropertyKey:
            case CubicDetachedVertexBase::outRotationPropertyKey:
            case CubicDetachedVertexBase::outDistancePropertyKey:
            case LayoutComponentBase::widthPropertyKey:
            case LayoutComponentBase::heightPropertyKey:
            case ArtboardBase::xPropertyKey:
            case ArtboardBase::yPropertyKey:
            case ArtboardBase::originXPropertyKey:
            case ArtboardBase::originYPropertyKey:
            case JoystickBase::xPropertyKey:
            case JoystickBase::yPropertyKey:
            case JoystickBase::posXPropertyKey:
            case JoystickBase::posYPropertyKey:
            case JoystickBase::originXPropertyKey:
            case JoystickBase::originYPropertyKey:
            case JoystickBase::widthPropertyKey:
            case JoystickBase::heightPropertyKey:
            case BoneBase::lengthPropertyKey:
            case RootBoneBase::xPropertyKey:
            case RootBoneBase::yPropertyKey:
            case SkinBase::xxPropertyKey:
            case SkinBase::yxPropertyKey:
            case SkinBase::xyPropertyKey:
            case SkinBase::yyPropertyKey:
            case SkinBase::txPropertyKey:
            case SkinBase::tyPropertyKey:
            case TendonBase::xxPropertyKey:
            case TendonBase::yxPropertyKey:
            case TendonBase::xyPropertyKey:
            case TendonBase::yyPropertyKey:
            case TendonBase::txPropertyKey:
            case TendonBase::tyPropertyKey:
            case TextModifierRangeBase::modifyFromPropertyKey:
            case TextModifierRangeBase::modifyToPropertyKey:
            case TextModifierRangeBase::strengthPropertyKey:
            case TextModifierRangeBase::falloffFromPropertyKey:
            case TextModifierRangeBase::falloffToPropertyKey:
            case TextModifierRangeBase::offsetPropertyKey:
            case TextVariationModifierBase::axisValuePropertyKey:
            case TextModifierGroupBase::originXPropertyKey:
            case TextModifierGroupBase::originYPropertyKey:
            case TextModifierGroupBase::opacityPropertyKey:
            case TextModifierGroupBase::xPropertyKey:
            case TextModifierGroupBase::yPropertyKey:
            case TextModifierGroupBase::rotationPropertyKey:
            case TextModifierGroupBase::scaleXPropertyKey:
            case TextModifierGroupBase::scaleYPropertyKey:
            case TextStyleBase::fontSizePropertyKey:
            case TextStyleBase::lineHeightPropertyKey:
            case TextStyleBase::letterSpacingPropertyKey:
            case TextStyleAxisBase::axisValuePropertyKey:
            case TextBase::widthPropertyKey:
            case TextBase::heightPropertyKey:
            case TextBase::originXPropertyKey:
            case TextBase::originYPropertyKey:
            case TextBase::paragraphSpacingPropertyKey:
            case DrawableAssetBase::heightPropertyKey:
            case DrawableAssetBase::widthPropertyKey:
            case ExportAudioBase::volumePropertyKey:
                return CoreDoubleType::id;
            case NestedArtboardBase::dataBindPathIdsPropertyKey:
            case MeshBase::triangleIndexBytesPropertyKey:
            case DataBindContextBase::sourcePathIdsPropertyKey:
            case FileAssetBase::cdnUuidPropertyKey:
            case FileAssetContentsBase::bytesPropertyKey:
                return CoreBytesType::id;
            default:
                return -1;
        }
    }
    static bool isCallback(uint32_t propertyKey)
    {
        switch (propertyKey)
        {
            case NestedTriggerBase::firePropertyKey:
            case EventBase::triggerPropertyKey:
                return true;
            default:
                return false;
        }
    }
    static bool objectSupportsProperty(Core* object, uint32_t propertyKey)
    {
        switch (propertyKey)
        {
            case ViewModelInstanceListItemBase::useLinkedArtboardPropertyKey:
                return object->is<ViewModelInstanceListItemBase>();
            case TransformComponentConstraintBase::offsetPropertyKey:
                return object->is<TransformComponentConstraintBase>();
            case TransformComponentConstraintBase::doesCopyPropertyKey:
                return object->is<TransformComponentConstraintBase>();
            case TransformComponentConstraintBase::minPropertyKey:
                return object->is<TransformComponentConstraintBase>();
            case TransformComponentConstraintBase::maxPropertyKey:
                return object->is<TransformComponentConstraintBase>();
            case TransformComponentConstraintYBase::doesCopyYPropertyKey:
                return object->is<TransformComponentConstraintYBase>();
            case TransformComponentConstraintYBase::minYPropertyKey:
                return object->is<TransformComponentConstraintYBase>();
            case TransformComponentConstraintYBase::maxYPropertyKey:
                return object->is<TransformComponentConstraintYBase>();
            case IKConstraintBase::invertDirectionPropertyKey:
                return object->is<IKConstraintBase>();
            case FollowPathConstraintBase::orientPropertyKey:
                return object->is<FollowPathConstraintBase>();
            case FollowPathConstraintBase::offsetPropertyKey:
                return object->is<FollowPathConstraintBase>();
            case NestedSimpleAnimationBase::isPlayingPropertyKey:
                return object->is<NestedSimpleAnimationBase>();
            case KeyFrameBoolBase::valuePropertyKey:
                return object->is<KeyFrameBoolBase>();
            case ListenerAlignTargetBase::preserveOffsetPropertyKey:
                return object->is<ListenerAlignTargetBase>();
            case NestedBoolBase::nestedValuePropertyKey:
                return object->is<NestedBoolBase>();
            case LinearAnimationBase::enableWorkAreaPropertyKey:
                return object->is<LinearAnimationBase>();
            case LinearAnimationBase::quantizePropertyKey:
                return object->is<LinearAnimationBase>();
            case StateMachineBoolBase::valuePropertyKey:
                return object->is<StateMachineBoolBase>();
            case ShapePaintBase::isVisiblePropertyKey:
                return object->is<ShapePaintBase>();
            case StrokeBase::transformAffectsStrokePropertyKey:
                return object->is<StrokeBase>();
            case PointsPathBase::isClosedPropertyKey:
                return object->is<PointsPathBase>();
            case RectangleBase::linkCornerRadiusPropertyKey:
                return object->is<RectangleBase>();
            case ClippingShapeBase::isVisiblePropertyKey:
                return object->is<ClippingShapeBase>();
            case CustomPropertyBooleanBase::propertyValuePropertyKey:
                return object->is<CustomPropertyBooleanBase>();
            case LayoutComponentBase::clipPropertyKey:
                return object->is<LayoutComponentBase>();
            case TextModifierRangeBase::clampPropertyKey:
                return object->is<TextModifierRangeBase>();
            case ViewModelInstanceListItemBase::viewModelIdPropertyKey:
                return object->is<ViewModelInstanceListItemBase>();
            case ViewModelInstanceListItemBase::viewModelInstanceIdPropertyKey:
                return object->is<ViewModelInstanceListItemBase>();
            case ViewModelInstanceListItemBase::artboardIdPropertyKey:
                return object->is<ViewModelInstanceListItemBase>();
            case ViewModelInstanceValueBase::viewModelPropertyIdPropertyKey:
                return object->is<ViewModelInstanceValueBase>();
            case ViewModelInstanceEnumBase::propertyValuePropertyKey:
                return object->is<ViewModelInstanceEnumBase>();
            case ViewModelBase::defaultInstanceIdPropertyKey:
                return object->is<ViewModelBase>();
            case ViewModelPropertyViewModelBase::viewModelReferenceIdPropertyKey:
                return object->is<ViewModelPropertyViewModelBase>();
            case ComponentBase::parentIdPropertyKey:
                return object->is<ComponentBase>();
            case ViewModelInstanceBase::viewModelIdPropertyKey:
                return object->is<ViewModelInstanceBase>();
            case ViewModelPropertyEnumBase::enumIdPropertyKey:
                return object->is<ViewModelPropertyEnumBase>();
            case ViewModelInstanceViewModelBase::propertyValuePropertyKey:
                return object->is<ViewModelInstanceViewModelBase>();
            case DrawTargetBase::drawableIdPropertyKey:
                return object->is<DrawTargetBase>();
            case DrawTargetBase::placementValuePropertyKey:
                return object->is<DrawTargetBase>();
            case TargetedConstraintBase::targetIdPropertyKey:
                return object->is<TargetedConstraintBase>();
            case DistanceConstraintBase::modeValuePropertyKey:
                return object->is<DistanceConstraintBase>();
            case TransformSpaceConstraintBase::sourceSpaceValuePropertyKey:
                return object->is<TransformSpaceConstraintBase>();
            case TransformSpaceConstraintBase::destSpaceValuePropertyKey:
                return object->is<TransformSpaceConstraintBase>();
            case TransformComponentConstraintBase::minMaxSpaceValuePropertyKey:
                return object->is<TransformComponentConstraintBase>();
            case IKConstraintBase::parentBoneCountPropertyKey:
                return object->is<IKConstraintBase>();
            case DrawableBase::blendModeValuePropertyKey:
                return object->is<DrawableBase>();
            case DrawableBase::drawableFlagsPropertyKey:
                return object->is<DrawableBase>();
            case NestedArtboardBase::artboardIdPropertyKey:
                return object->is<NestedArtboardBase>();
            case NestedArtboardBase::fitPropertyKey:
                return object->is<NestedArtboardBase>();
            case NestedArtboardBase::alignmentPropertyKey:
                return object->is<NestedArtboardBase>();
            case NestedAnimationBase::animationIdPropertyKey:
                return object->is<NestedAnimationBase>();
            case SoloBase::activeComponentIdPropertyKey:
                return object->is<SoloBase>();
            case LayoutComponentStyleBase::layoutFlags0PropertyKey:
                return object->is<LayoutComponentStyleBase>();
            case LayoutComponentStyleBase::layoutFlags1PropertyKey:
                return object->is<LayoutComponentStyleBase>();
            case LayoutComponentStyleBase::layoutFlags2PropertyKey:
                return object->is<LayoutComponentStyleBase>();
            case ListenerFireEventBase::eventIdPropertyKey:
                return object->is<ListenerFireEventBase>();
            case LayerStateBase::flagsPropertyKey:
                return object->is<LayerStateBase>();
            case ListenerInputChangeBase::inputIdPropertyKey:
                return object->is<ListenerInputChangeBase>();
            case ListenerInputChangeBase::nestedInputIdPropertyKey:
                return object->is<ListenerInputChangeBase>();
            case AnimationStateBase::animationIdPropertyKey:
                return object->is<AnimationStateBase>();
            case NestedInputBase::inputIdPropertyKey:
                return object->is<NestedInputBase>();
            case KeyedObjectBase::objectIdPropertyKey:
                return object->is<KeyedObjectBase>();
            case BlendAnimationBase::animationIdPropertyKey:
                return object->is<BlendAnimationBase>();
            case BlendAnimationDirectBase::inputIdPropertyKey:
                return object->is<BlendAnimationDirectBase>();
            case BlendAnimationDirectBase::blendSourcePropertyKey:
                return object->is<BlendAnimationDirectBase>();
            case TransitionConditionBase::inputIdPropertyKey:
                return object->is<TransitionConditionBase>();
            case KeyedPropertyBase::propertyKeyPropertyKey:
                return object->is<KeyedPropertyBase>();
            case StateMachineListenerBase::targetIdPropertyKey:
                return object->is<StateMachineListenerBase>();
            case StateMachineListenerBase::listenerTypeValuePropertyKey:
                return object->is<StateMachineListenerBase>();
            case StateMachineListenerBase::eventIdPropertyKey:
                return object->is<StateMachineListenerBase>();
            case KeyFrameBase::framePropertyKey:
                return object->is<KeyFrameBase>();
            case InterpolatingKeyFrameBase::interpolationTypePropertyKey:
                return object->is<InterpolatingKeyFrameBase>();
            case InterpolatingKeyFrameBase::interpolatorIdPropertyKey:
                return object->is<InterpolatingKeyFrameBase>();
            case KeyFrameIdBase::valuePropertyKey:
                return object->is<KeyFrameIdBase>();
            case ListenerBoolChangeBase::valuePropertyKey:
                return object->is<ListenerBoolChangeBase>();
            case ListenerAlignTargetBase::targetIdPropertyKey:
                return object->is<ListenerAlignTargetBase>();
            case TransitionValueConditionBase::opValuePropertyKey:
                return object->is<TransitionValueConditionBase>();
            case StateTransitionBase::stateToIdPropertyKey:
                return object->is<StateTransitionBase>();
            case StateTransitionBase::flagsPropertyKey:
                return object->is<StateTransitionBase>();
            case StateTransitionBase::durationPropertyKey:
                return object->is<StateTransitionBase>();
            case StateTransitionBase::exitTimePropertyKey:
                return object->is<StateTransitionBase>();
            case StateTransitionBase::interpolationTypePropertyKey:
                return object->is<StateTransitionBase>();
            case StateTransitionBase::interpolatorIdPropertyKey:
                return object->is<StateTransitionBase>();
            case StateTransitionBase::randomWeightPropertyKey:
                return object->is<StateTransitionBase>();
            case StateMachineFireEventBase::eventIdPropertyKey:
                return object->is<StateMachineFireEventBase>();
            case StateMachineFireEventBase::occursValuePropertyKey:
                return object->is<StateMachineFireEventBase>();
            case LinearAnimationBase::fpsPropertyKey:
                return object->is<LinearAnimationBase>();
            case LinearAnimationBase::durationPropertyKey:
                return object->is<LinearAnimationBase>();
            case LinearAnimationBase::loopValuePropertyKey:
                return object->is<LinearAnimationBase>();
            case LinearAnimationBase::workStartPropertyKey:
                return object->is<LinearAnimationBase>();
            case LinearAnimationBase::workEndPropertyKey:
                return object->is<LinearAnimationBase>();
            case ElasticInterpolatorBase::easingValuePropertyKey:
                return object->is<ElasticInterpolatorBase>();
            case BlendState1DBase::inputIdPropertyKey:
                return object->is<BlendState1DBase>();
            case BlendStateTransitionBase::exitBlendAnimationIdPropertyKey:
                return object->is<BlendStateTransitionBase>();
            case StrokeBase::capPropertyKey:
                return object->is<StrokeBase>();
            case StrokeBase::joinPropertyKey:
                return object->is<StrokeBase>();
            case TrimPathBase::modeValuePropertyKey:
                return object->is<TrimPathBase>();
            case FillBase::fillRulePropertyKey:
                return object->is<FillBase>();
            case PathBase::pathFlagsPropertyKey:
                return object->is<PathBase>();
            case ClippingShapeBase::sourceIdPropertyKey:
                return object->is<ClippingShapeBase>();
            case ClippingShapeBase::fillRulePropertyKey:
                return object->is<ClippingShapeBase>();
            case PolygonBase::pointsPropertyKey:
                return object->is<PolygonBase>();
            case ImageBase::assetIdPropertyKey:
                return object->is<ImageBase>();
            case DrawRulesBase::drawTargetIdPropertyKey:
                return object->is<DrawRulesBase>();
            case LayoutComponentBase::styleIdPropertyKey:
                return object->is<LayoutComponentBase>();
            case ArtboardBase::defaultStateMachineIdPropertyKey:
                return object->is<ArtboardBase>();
            case ArtboardBase::viewModelIdPropertyKey:
                return object->is<ArtboardBase>();
            case JoystickBase::xIdPropertyKey:
                return object->is<JoystickBase>();
            case JoystickBase::yIdPropertyKey:
                return object->is<JoystickBase>();
            case JoystickBase::joystickFlagsPropertyKey:
                return object->is<JoystickBase>();
            case JoystickBase::handleSourceIdPropertyKey:
                return object->is<JoystickBase>();
            case OpenUrlEventBase::targetValuePropertyKey:
                return object->is<OpenUrlEventBase>();
            case DataBindBase::targetIdPropertyKey:
                return object->is<DataBindBase>();
            case DataBindBase::propertyKeyPropertyKey:
                return object->is<DataBindBase>();
            case DataBindBase::modeValuePropertyKey:
                return object->is<DataBindBase>();
            case WeightBase::valuesPropertyKey:
                return object->is<WeightBase>();
            case WeightBase::indicesPropertyKey:
                return object->is<WeightBase>();
            case TendonBase::boneIdPropertyKey:
                return object->is<TendonBase>();
            case CubicWeightBase::inValuesPropertyKey:
                return object->is<CubicWeightBase>();
            case CubicWeightBase::inIndicesPropertyKey:
                return object->is<CubicWeightBase>();
            case CubicWeightBase::outValuesPropertyKey:
                return object->is<CubicWeightBase>();
            case CubicWeightBase::outIndicesPropertyKey:
                return object->is<CubicWeightBase>();
            case TextModifierRangeBase::unitsValuePropertyKey:
                return object->is<TextModifierRangeBase>();
            case TextModifierRangeBase::typeValuePropertyKey:
                return object->is<TextModifierRangeBase>();
            case TextModifierRangeBase::modeValuePropertyKey:
                return object->is<TextModifierRangeBase>();
            case TextModifierRangeBase::runIdPropertyKey:
                return object->is<TextModifierRangeBase>();
            case TextStyleFeatureBase::tagPropertyKey:
                return object->is<TextStyleFeatureBase>();
            case TextStyleFeatureBase::featureValuePropertyKey:
                return object->is<TextStyleFeatureBase>();
            case TextVariationModifierBase::axisTagPropertyKey:
                return object->is<TextVariationModifierBase>();
            case TextModifierGroupBase::modifierFlagsPropertyKey:
                return object->is<TextModifierGroupBase>();
            case TextStyleBase::fontAssetIdPropertyKey:
                return object->is<TextStyleBase>();
            case TextStyleAxisBase::tagPropertyKey:
                return object->is<TextStyleAxisBase>();
            case TextBase::alignValuePropertyKey:
                return object->is<TextBase>();
            case TextBase::sizingValuePropertyKey:
                return object->is<TextBase>();
            case TextBase::overflowValuePropertyKey:
                return object->is<TextBase>();
            case TextBase::originValuePropertyKey:
                return object->is<TextBase>();
            case TextValueRunBase::styleIdPropertyKey:
                return object->is<TextValueRunBase>();
            case FileAssetBase::assetIdPropertyKey:
                return object->is<FileAssetBase>();
            case AudioEventBase::assetIdPropertyKey:
                return object->is<AudioEventBase>();
            case ViewModelInstanceColorBase::propertyValuePropertyKey:
                return object->is<ViewModelInstanceColorBase>();
            case KeyFrameColorBase::valuePropertyKey:
                return object->is<KeyFrameColorBase>();
            case SolidColorBase::colorValuePropertyKey:
                return object->is<SolidColorBase>();
            case GradientStopBase::colorValuePropertyKey:
                return object->is<GradientStopBase>();
            case ViewModelComponentBase::namePropertyKey:
                return object->is<ViewModelComponentBase>();
            case ViewModelInstanceStringBase::propertyValuePropertyKey:
                return object->is<ViewModelInstanceStringBase>();
            case ComponentBase::namePropertyKey:
                return object->is<ComponentBase>();
            case DataEnumValueBase::keyPropertyKey:
                return object->is<DataEnumValueBase>();
            case DataEnumValueBase::valuePropertyKey:
                return object->is<DataEnumValueBase>();
            case AnimationBase::namePropertyKey:
                return object->is<AnimationBase>();
            case StateMachineComponentBase::namePropertyKey:
                return object->is<StateMachineComponentBase>();
            case KeyFrameStringBase::valuePropertyKey:
                return object->is<KeyFrameStringBase>();
            case OpenUrlEventBase::urlPropertyKey:
                return object->is<OpenUrlEventBase>();
            case TextValueRunBase::textPropertyKey:
                return object->is<TextValueRunBase>();
            case CustomPropertyStringBase::propertyValuePropertyKey:
                return object->is<CustomPropertyStringBase>();
            case AssetBase::namePropertyKey:
                return object->is<AssetBase>();
            case FileAssetBase::cdnBaseUrlPropertyKey:
                return object->is<FileAssetBase>();
            case ViewModelInstanceNumberBase::propertyValuePropertyKey:
                return object->is<ViewModelInstanceNumberBase>();
            case CustomPropertyNumberBase::propertyValuePropertyKey:
                return object->is<CustomPropertyNumberBase>();
            case ConstraintBase::strengthPropertyKey:
                return object->is<ConstraintBase>();
            case DistanceConstraintBase::distancePropertyKey:
                return object->is<DistanceConstraintBase>();
            case TransformComponentConstraintBase::copyFactorPropertyKey:
                return object->is<TransformComponentConstraintBase>();
            case TransformComponentConstraintBase::minValuePropertyKey:
                return object->is<TransformComponentConstraintBase>();
            case TransformComponentConstraintBase::maxValuePropertyKey:
                return object->is<TransformComponentConstraintBase>();
            case TransformComponentConstraintYBase::copyFactorYPropertyKey:
                return object->is<TransformComponentConstraintYBase>();
            case TransformComponentConstraintYBase::minValueYPropertyKey:
                return object->is<TransformComponentConstraintYBase>();
            case TransformComponentConstraintYBase::maxValueYPropertyKey:
                return object->is<TransformComponentConstraintYBase>();
            case FollowPathConstraintBase::distancePropertyKey:
                return object->is<FollowPathConstraintBase>();
            case TransformConstraintBase::originXPropertyKey:
                return object->is<TransformConstraintBase>();
            case TransformConstraintBase::originYPropertyKey:
                return object->is<TransformConstraintBase>();
            case WorldTransformComponentBase::opacityPropertyKey:
                return object->is<WorldTransformComponentBase>();
            case TransformComponentBase::rotationPropertyKey:
                return object->is<TransformComponentBase>();
            case TransformComponentBase::scaleXPropertyKey:
                return object->is<TransformComponentBase>();
            case TransformComponentBase::scaleYPropertyKey:
                return object->is<TransformComponentBase>();
            case NodeBase::xPropertyKey:
                return object->is<NodeBase>();
            case NodeBase::yPropertyKey:
                return object->is<NodeBase>();
            case LayoutComponentStyleBase::gapHorizontalPropertyKey:
                return object->is<LayoutComponentStyleBase>();
            case LayoutComponentStyleBase::gapVerticalPropertyKey:
                return object->is<LayoutComponentStyleBase>();
            case LayoutComponentStyleBase::maxWidthPropertyKey:
                return object->is<LayoutComponentStyleBase>();
            case LayoutComponentStyleBase::maxHeightPropertyKey:
                return object->is<LayoutComponentStyleBase>();
            case LayoutComponentStyleBase::minWidthPropertyKey:
                return object->is<LayoutComponentStyleBase>();
            case LayoutComponentStyleBase::minHeightPropertyKey:
                return object->is<LayoutComponentStyleBase>();
            case LayoutComponentStyleBase::borderLeftPropertyKey:
                return object->is<LayoutComponentStyleBase>();
            case LayoutComponentStyleBase::borderRightPropertyKey:
                return object->is<LayoutComponentStyleBase>();
            case LayoutComponentStyleBase::borderTopPropertyKey:
                return object->is<LayoutComponentStyleBase>();
            case LayoutComponentStyleBase::borderBottomPropertyKey:
                return object->is<LayoutComponentStyleBase>();
            case LayoutComponentStyleBase::marginLeftPropertyKey:
                return object->is<LayoutComponentStyleBase>();
            case LayoutComponentStyleBase::marginRightPropertyKey:
                return object->is<LayoutComponentStyleBase>();
            case LayoutComponentStyleBase::marginTopPropertyKey:
                return object->is<LayoutComponentStyleBase>();
            case LayoutComponentStyleBase::marginBottomPropertyKey:
                return object->is<LayoutComponentStyleBase>();
            case LayoutComponentStyleBase::paddingLeftPropertyKey:
                return object->is<LayoutComponentStyleBase>();
            case LayoutComponentStyleBase::paddingRightPropertyKey:
                return object->is<LayoutComponentStyleBase>();
            case LayoutComponentStyleBase::paddingTopPropertyKey:
                return object->is<LayoutComponentStyleBase>();
            case LayoutComponentStyleBase::paddingBottomPropertyKey:
                return object->is<LayoutComponentStyleBase>();
            case LayoutComponentStyleBase::positionLeftPropertyKey:
                return object->is<LayoutComponentStyleBase>();
            case LayoutComponentStyleBase::positionRightPropertyKey:
                return object->is<LayoutComponentStyleBase>();
            case LayoutComponentStyleBase::positionTopPropertyKey:
                return object->is<LayoutComponentStyleBase>();
            case LayoutComponentStyleBase::positionBottomPropertyKey:
                return object->is<LayoutComponentStyleBase>();
            case LayoutComponentStyleBase::flexPropertyKey:
                return object->is<LayoutComponentStyleBase>();
            case LayoutComponentStyleBase::flexGrowPropertyKey:
                return object->is<LayoutComponentStyleBase>();
            case LayoutComponentStyleBase::flexShrinkPropertyKey:
                return object->is<LayoutComponentStyleBase>();
            case LayoutComponentStyleBase::flexBasisPropertyKey:
                return object->is<LayoutComponentStyleBase>();
            case LayoutComponentStyleBase::aspectRatioPropertyKey:
                return object->is<LayoutComponentStyleBase>();
            case NestedLinearAnimationBase::mixPropertyKey:
                return object->is<NestedLinearAnimationBase>();
            case NestedSimpleAnimationBase::speedPropertyKey:
                return object->is<NestedSimpleAnimationBase>();
            case AdvanceableStateBase::speedPropertyKey:
                return object->is<AdvanceableStateBase>();
            case BlendAnimationDirectBase::mixValuePropertyKey:
                return object->is<BlendAnimationDirectBase>();
            case StateMachineNumberBase::valuePropertyKey:
                return object->is<StateMachineNumberBase>();
            case CubicInterpolatorBase::x1PropertyKey:
                return object->is<CubicInterpolatorBase>();
            case CubicInterpolatorBase::y1PropertyKey:
                return object->is<CubicInterpolatorBase>();
            case CubicInterpolatorBase::x2PropertyKey:
                return object->is<CubicInterpolatorBase>();
            case CubicInterpolatorBase::y2PropertyKey:
                return object->is<CubicInterpolatorBase>();
            case TransitionNumberConditionBase::valuePropertyKey:
                return object->is<TransitionNumberConditionBase>();
            case CubicInterpolatorComponentBase::x1PropertyKey:
                return object->is<CubicInterpolatorComponentBase>();
            case CubicInterpolatorComponentBase::y1PropertyKey:
                return object->is<CubicInterpolatorComponentBase>();
            case CubicInterpolatorComponentBase::x2PropertyKey:
                return object->is<CubicInterpolatorComponentBase>();
            case CubicInterpolatorComponentBase::y2PropertyKey:
                return object->is<CubicInterpolatorComponentBase>();
            case ListenerNumberChangeBase::valuePropertyKey:
                return object->is<ListenerNumberChangeBase>();
            case KeyFrameDoubleBase::valuePropertyKey:
                return object->is<KeyFrameDoubleBase>();
            case LinearAnimationBase::speedPropertyKey:
                return object->is<LinearAnimationBase>();
            case ElasticInterpolatorBase::amplitudePropertyKey:
                return object->is<ElasticInterpolatorBase>();
            case ElasticInterpolatorBase::periodPropertyKey:
                return object->is<ElasticInterpolatorBase>();
            case NestedNumberBase::nestedValuePropertyKey:
                return object->is<NestedNumberBase>();
            case NestedRemapAnimationBase::timePropertyKey:
                return object->is<NestedRemapAnimationBase>();
            case BlendAnimation1DBase::valuePropertyKey:
                return object->is<BlendAnimation1DBase>();
            case LinearGradientBase::startXPropertyKey:
                return object->is<LinearGradientBase>();
            case LinearGradientBase::startYPropertyKey:
                return object->is<LinearGradientBase>();
            case LinearGradientBase::endXPropertyKey:
                return object->is<LinearGradientBase>();
            case LinearGradientBase::endYPropertyKey:
                return object->is<LinearGradientBase>();
            case LinearGradientBase::opacityPropertyKey:
                return object->is<LinearGradientBase>();
            case StrokeBase::thicknessPropertyKey:
                return object->is<StrokeBase>();
            case GradientStopBase::positionPropertyKey:
                return object->is<GradientStopBase>();
            case TrimPathBase::startPropertyKey:
                return object->is<TrimPathBase>();
            case TrimPathBase::endPropertyKey:
                return object->is<TrimPathBase>();
            case TrimPathBase::offsetPropertyKey:
                return object->is<TrimPathBase>();
            case VertexBase::xPropertyKey:
                return object->is<VertexBase>();
            case VertexBase::yPropertyKey:
                return object->is<VertexBase>();
            case MeshVertexBase::uPropertyKey:
                return object->is<MeshVertexBase>();
            case MeshVertexBase::vPropertyKey:
                return object->is<MeshVertexBase>();
            case StraightVertexBase::radiusPropertyKey:
                return object->is<StraightVertexBase>();
            case CubicAsymmetricVertexBase::rotationPropertyKey:
                return object->is<CubicAsymmetricVertexBase>();
            case CubicAsymmetricVertexBase::inDistancePropertyKey:
                return object->is<CubicAsymmetricVertexBase>();
            case CubicAsymmetricVertexBase::outDistancePropertyKey:
                return object->is<CubicAsymmetricVertexBase>();
            case ParametricPathBase::widthPropertyKey:
                return object->is<ParametricPathBase>();
            case ParametricPathBase::heightPropertyKey:
                return object->is<ParametricPathBase>();
            case ParametricPathBase::originXPropertyKey:
                return object->is<ParametricPathBase>();
            case ParametricPathBase::originYPropertyKey:
                return object->is<ParametricPathBase>();
            case RectangleBase::cornerRadiusTLPropertyKey:
                return object->is<RectangleBase>();
            case RectangleBase::cornerRadiusTRPropertyKey:
                return object->is<RectangleBase>();
            case RectangleBase::cornerRadiusBLPropertyKey:
                return object->is<RectangleBase>();
            case RectangleBase::cornerRadiusBRPropertyKey:
                return object->is<RectangleBase>();
            case CubicMirroredVertexBase::rotationPropertyKey:
                return object->is<CubicMirroredVertexBase>();
            case CubicMirroredVertexBase::distancePropertyKey:
                return object->is<CubicMirroredVertexBase>();
            case PolygonBase::cornerRadiusPropertyKey:
                return object->is<PolygonBase>();
            case StarBase::innerRadiusPropertyKey:
                return object->is<StarBase>();
            case ImageBase::originXPropertyKey:
                return object->is<ImageBase>();
            case ImageBase::originYPropertyKey:
                return object->is<ImageBase>();
            case CubicDetachedVertexBase::inRotationPropertyKey:
                return object->is<CubicDetachedVertexBase>();
            case CubicDetachedVertexBase::inDistancePropertyKey:
                return object->is<CubicDetachedVertexBase>();
            case CubicDetachedVertexBase::outRotationPropertyKey:
                return object->is<CubicDetachedVertexBase>();
            case CubicDetachedVertexBase::outDistancePropertyKey:
                return object->is<CubicDetachedVertexBase>();
            case LayoutComponentBase::widthPropertyKey:
                return object->is<LayoutComponentBase>();
            case LayoutComponentBase::heightPropertyKey:
                return object->is<LayoutComponentBase>();
            case ArtboardBase::xPropertyKey:
                return object->is<ArtboardBase>();
            case ArtboardBase::yPropertyKey:
                return object->is<ArtboardBase>();
            case ArtboardBase::originXPropertyKey:
                return object->is<ArtboardBase>();
            case ArtboardBase::originYPropertyKey:
                return object->is<ArtboardBase>();
            case JoystickBase::xPropertyKey:
                return object->is<JoystickBase>();
            case JoystickBase::yPropertyKey:
                return object->is<JoystickBase>();
            case JoystickBase::posXPropertyKey:
                return object->is<JoystickBase>();
            case JoystickBase::posYPropertyKey:
                return object->is<JoystickBase>();
            case JoystickBase::originXPropertyKey:
                return object->is<JoystickBase>();
            case JoystickBase::originYPropertyKey:
                return object->is<JoystickBase>();
            case JoystickBase::widthPropertyKey:
                return object->is<JoystickBase>();
            case JoystickBase::heightPropertyKey:
                return object->is<JoystickBase>();
            case BoneBase::lengthPropertyKey:
                return object->is<BoneBase>();
            case RootBoneBase::xPropertyKey:
                return object->is<RootBoneBase>();
            case RootBoneBase::yPropertyKey:
                return object->is<RootBoneBase>();
            case SkinBase::xxPropertyKey:
                return object->is<SkinBase>();
            case SkinBase::yxPropertyKey:
                return object->is<SkinBase>();
            case SkinBase::xyPropertyKey:
                return object->is<SkinBase>();
            case SkinBase::yyPropertyKey:
                return object->is<SkinBase>();
            case SkinBase::txPropertyKey:
                return object->is<SkinBase>();
            case SkinBase::tyPropertyKey:
                return object->is<SkinBase>();
            case TendonBase::xxPropertyKey:
                return object->is<TendonBase>();
            case TendonBase::yxPropertyKey:
                return object->is<TendonBase>();
            case TendonBase::xyPropertyKey:
                return object->is<TendonBase>();
            case TendonBase::yyPropertyKey:
                return object->is<TendonBase>();
            case TendonBase::txPropertyKey:
                return object->is<TendonBase>();
            case TendonBase::tyPropertyKey:
                return object->is<TendonBase>();
            case TextModifierRangeBase::modifyFromPropertyKey:
                return object->is<TextModifierRangeBase>();
            case TextModifierRangeBase::modifyToPropertyKey:
                return object->is<TextModifierRangeBase>();
            case TextModifierRangeBase::strengthPropertyKey:
                return object->is<TextModifierRangeBase>();
            case TextModifierRangeBase::falloffFromPropertyKey:
                return object->is<TextModifierRangeBase>();
            case TextModifierRangeBase::falloffToPropertyKey:
                return object->is<TextModifierRangeBase>();
            case TextModifierRangeBase::offsetPropertyKey:
                return object->is<TextModifierRangeBase>();
            case TextVariationModifierBase::axisValuePropertyKey:
                return object->is<TextVariationModifierBase>();
            case TextModifierGroupBase::originXPropertyKey:
                return object->is<TextModifierGroupBase>();
            case TextModifierGroupBase::originYPropertyKey:
                return object->is<TextModifierGroupBase>();
            case TextModifierGroupBase::opacityPropertyKey:
                return object->is<TextModifierGroupBase>();
            case TextModifierGroupBase::xPropertyKey:
                return object->is<TextModifierGroupBase>();
            case TextModifierGroupBase::yPropertyKey:
                return object->is<TextModifierGroupBase>();
            case TextModifierGroupBase::rotationPropertyKey:
                return object->is<TextModifierGroupBase>();
            case TextModifierGroupBase::scaleXPropertyKey:
                return object->is<TextModifierGroupBase>();
            case TextModifierGroupBase::scaleYPropertyKey:
                return object->is<TextModifierGroupBase>();
            case TextStyleBase::fontSizePropertyKey:
                return object->is<TextStyleBase>();
            case TextStyleBase::lineHeightPropertyKey:
                return object->is<TextStyleBase>();
            case TextStyleBase::letterSpacingPropertyKey:
                return object->is<TextStyleBase>();
            case TextStyleAxisBase::axisValuePropertyKey:
                return object->is<TextStyleAxisBase>();
            case TextBase::widthPropertyKey:
                return object->is<TextBase>();
            case TextBase::heightPropertyKey:
                return object->is<TextBase>();
            case TextBase::originXPropertyKey:
                return object->is<TextBase>();
            case TextBase::originYPropertyKey:
                return object->is<TextBase>();
            case TextBase::paragraphSpacingPropertyKey:
                return object->is<TextBase>();
            case DrawableAssetBase::heightPropertyKey:
                return object->is<DrawableAssetBase>();
            case DrawableAssetBase::widthPropertyKey:
                return object->is<DrawableAssetBase>();
            case ExportAudioBase::volumePropertyKey:
                return object->is<ExportAudioBase>();
            case NestedTriggerBase::firePropertyKey:
                return object->is<NestedTriggerBase>();
            case EventBase::triggerPropertyKey:
                return object->is<EventBase>();
        }
        return false;
    }
};
} // namespace rive

#endif