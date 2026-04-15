## Running

as there are 3 parts to this project, there are 3 environments which need to be manually selected:

- `pc`
- `router`
- `aggregator`
- `sensor`

this can be done by passing `--environment` to `pio`/`platformio` commands like this:

```bash
pio run --environment <env_name_goes_here>
```

> [!TIP]
> All environment can be targeted by omitting flag. E.g. for compiling all:
> ```bash
> pio run
> ```
