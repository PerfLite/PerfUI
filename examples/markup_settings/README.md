# Markup Settings Menu Example

This example demonstrates how to build and interact with a Skyrim mod configuration menu declared in XML (`settings.xml`) using `PerfUI_Markup`.

## Features Demonstrated

1. **Declarative XML Layout:** Nested panels, sliders, checkboxes, combo boxes, and buttons defined with zero C++ layout code.
2. **Named Callback Binding:** C++ event lambdas bound cleanly via `loader.bindCallback(...)`.
3. **Widget Querying:** Dynamic runtime querying by element name (`loader.findByName("MasterVolume")`).
4. **Instant Hot-Reload:** Edit `settings.xml` while running to see layout and style changes immediately without restarting or losing state.

## XML Snippet

```xml
<UI>
  <Panel name="SettingsWindow" direction="column" width="460" padding="20" gap="14">
    <Text text="Mod Configuration Menu" font="$fontTitle" color="$nordicGold"/>
    <Slider name="MasterVolume" label="Master Volume" min="0" max="100" value="80"/>
    <Checkbox name="EnableCompass" label="Show Extended Enemy Compass" checked="true"/>
    <Panel direction="row" gap="10" justify="end">
      <Button text="Cancel" onClick="cancelAction"/>
      <Button text="Apply" onClick="applyAction" class="primary"/>
    </Panel>
  </Panel>
</UI>
```

See `docs/MARKUP.md` for the complete markup reference and available tokens.
