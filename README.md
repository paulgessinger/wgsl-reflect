# wgsl-reflect

## Limitations

- Attribute values are not evaluated, i.e. if the value is not a literal but a *[
  const-expression](https://www.w3.org/TR/WGSL/#const-expressions)*, `wgsl_reflect`
  will
  just give you the expression itself