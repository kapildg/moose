/****************************************************************/
/* MOOSE - Multiphysics Object Oriented Simulation Environment  */
/*                                                              */
/*          All contents are licensed under LGPL V2.1           */
/*             See LICENSE for full restrictions                */
/****************************************************************/

#include "PorousFlowEffectiveStressCoupling.h"

// MOOSE includes
#include "Function.h"
#include "MooseMesh.h"
#include "MooseVariable.h"

template <>
InputParameters
validParams<PorousFlowEffectiveStressCoupling>()
{
  InputParameters params = validParams<Kernel>();
  params.addClassDescription("Adds -BiotCoefficient*effective_porepressure*grad_test[component]");
  params.addRequiredParam<UserObjectName>(
      "PorousFlowDictator", "The UserObject that holds the list of Porous-Flow variable names.");
  params.addRangeCheckedParam<Real>(
      "biot_coefficient", 1, "biot_coefficient>=0&biot_coefficient<=1", "Biot coefficient");
  params.addRequiredParam<unsigned int>("component",
                                        "The gradient direction (0 for x, 1 for y and 2 for z)");
  return params;
}

PorousFlowEffectiveStressCoupling::PorousFlowEffectiveStressCoupling(
    const InputParameters & parameters)
  : Kernel(parameters),
    _dictator(getUserObject<PorousFlowDictator>("PorousFlowDictator")),
    _coefficient(getParam<Real>("biot_coefficient")),
    _component(getParam<unsigned int>("component")),
    _pf(getMaterialProperty<Real>("PorousFlow_effective_fluid_pressure_qp")),
    _dpf_dvar(
        getMaterialProperty<std::vector<Real>>("dPorousFlow_effective_fluid_pressure_qp_dvar")),
    _rz(getBlockCoordSystem() == Moose::COORD_RZ)
{
  if (_component >= _mesh.dimension())
    mooseError("PorousFlowEffectiveStressCoupling: component should not be greater than the mesh "
               "dimension");
}

Real
PorousFlowEffectiveStressCoupling::computeQpResidual()
{
  if (_rz && _component == 0)
    return -_coefficient * _pf[_qp] * (_grad_test[_i][_qp](0) + _test[_i][_qp] / _q_point[_qp](0));
  return -_coefficient * _pf[_qp] * _grad_test[_i][_qp](_component);
}

Real
PorousFlowEffectiveStressCoupling::computeQpJacobian()
{
  if (_dictator.notPorousFlowVariable(_var.number()))
    return 0.0;
  const unsigned int pvar = _dictator.porousFlowVariableNum(_var.number());
  if (_rz && _component == 0)
    return -_coefficient * _phi[_j][_qp] * _dpf_dvar[_qp][pvar] *
           (_grad_test[_i][_qp](0) + _test[_i][_qp] / _q_point[_qp](0));
  return -_coefficient * _phi[_j][_qp] * _dpf_dvar[_qp][pvar] * _grad_test[_i][_qp](_component);
}

Real
PorousFlowEffectiveStressCoupling::computeQpOffDiagJacobian(unsigned int jvar)
{
  if (_dictator.notPorousFlowVariable(jvar))
    return 0.0;
  const unsigned int pvar = _dictator.porousFlowVariableNum(jvar);
  if (_rz && _component == 0)
    return -_coefficient * _phi[_j][_qp] * _dpf_dvar[_qp][pvar] *
           (_grad_test[_i][_qp](0) + _test[_i][_qp] / _q_point[_qp](0));
  return -_coefficient * _phi[_j][_qp] * _dpf_dvar[_qp][pvar] * _grad_test[_i][_qp](_component);
}
