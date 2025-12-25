# Otto API Documentation

This directory contains API documentation for the Otto Job Scheduler system.

## Files

- `otto-api.yaml` - OpenAPI 3.0 specification for the Otto REST API

## Usage

### View the API Documentation

You can view and interact with the API documentation using any OpenAPI-compatible tool:

**Online Swagger Editor:**
1. Go to https://editor.swagger.io/
2. Copy the contents of `otto-api.yaml` and paste it into the editor

**Local with Swagger UI:**
```bash
# Using Docker
docker run -p 8081:8080 -v $(pwd)/docs:/usr/share/nginx/html/docs swaggerapi/swagger-ui

# Then navigate to: http://localhost:8081/?url=/docs/otto-api.yaml
```

**Redoc:**
```bash
# Using npx
npx redoc-cli serve otto-api.yaml --watch
```

### API Endpoint

The main JSON endpoint is:
```
GET /otto/json?jobname={pattern}&radiofmt=on&IN=1&AC=1&OH=1&RU=1&SU=1&FA=1&TE=1
```

### Parameters

- `jobname` - Filter jobs by name pattern (supports `%` and `_` wildcards)
- `radiofmt` - Legacy compatibility parameter (`on`/`off`)
- Status filters (`0`/`1`):
  - `IN` - Include INACTIVE jobs
  - `AC` - Include ACTIVE jobs  
  - `OH` - Include ON_HOLD jobs
  - `RU` - Include RUNNING jobs
  - `SU` - Include SUCCESS jobs
  - `FA` - Include FAILURE jobs
  - `TE` - Include TERMINATED jobs

### Response Format

Returns JSON with `schema_version: 2`:
- Numeric timestamps (Unix epoch seconds)
- Numeric durations and status codes
- Boolean flags as 0/1 integers
- Proper JSON string escaping
- Hierarchical job structure with boxes containing child jobs

### Examples

Get all jobs:
```bash
curl "http://localhost:8080/otto/json"
```

Get only running and failed jobs:
```bash
curl "http://localhost:8080/otto/json?RU=1&FA=1&IN=0&AC=0&OH=0&SU=0&TE=0"
```

Get jobs matching pattern:
```bash
curl "http://localhost:8080/otto/json?jobname=batch_%"
```