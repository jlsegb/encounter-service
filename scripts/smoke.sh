#!/usr/bin/env bash
set -euo pipefail

BASE_URL="${BASE_URL:-http://localhost:8080}"
API_KEY="${API_KEY:-smoke-user}"
AUDIT_FROM_DATE_UTC="${AUDIT_FROM_DATE_UTC:-$(date -u +%F)}"
AUDIT_TO_DATE_UTC="${AUDIT_TO_DATE_UTC:-$(date -u -v+1d +%F 2>/dev/null || date -u -d '+1 day' +%F)}"

have_jq() { command -v jq >/dev/null 2>&1; }

echo "== Health =="
curl -sS -i "$BASE_URL/health" | sed -n '1,5p'
echo

echo "== Unauthorized check (expect 401) =="
curl -sS -i "$BASE_URL/encounters" | sed -n '1,20p'
echo

echo "== Create encounter (expect 201) =="
CREATE_RESP=$(curl -sS \
  -H "Content-Type: application/json" \
  -H "X-API-Key: $API_KEY" \
  -X POST "$BASE_URL/encounters" \
  -d '{
    "patientId":"P123",
    "providerId":"T456",
    "encounterDate":"2024-10-15T14:30:00Z",
    "encounterType":"initial_assessment",
    "clinicalData":{"note":"smoke test","score":7}
  }')

echo "$CREATE_RESP"
echo

if have_jq; then
  ENCOUNTER_ID=$(echo "$CREATE_RESP" | jq -r '.encounterId // empty')
else
  # crude fallback: expects "encounterId":"..."
  ENCOUNTER_ID=$(echo "$CREATE_RESP" | sed -n 's/.*"encounterId"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p')
fi

if [[ -z "${ENCOUNTER_ID:-}" ]]; then
  echo "ERROR: Could not extract encounterId from create response."
  exit 1
fi

echo "Encounter ID: $ENCOUNTER_ID"
echo

echo "== Get encounter (expect 200) =="
curl -sS -i -H "X-API-Key: $API_KEY" "$BASE_URL/encounters/$ENCOUNTER_ID" | sed -n '1,25p'
echo

echo "== List encounters (expect 200) =="
curl -sS -i -H "X-API-Key: $API_KEY" \
  "$BASE_URL/encounters?providerId=T456&from=2024-10-01&to=2024-10-31" | sed -n '1,40p'
echo

echo "== Audit encounters (expect CREATE + READ) =="
echo "Audit range (UTC): $AUDIT_FROM_DATE_UTC -> $AUDIT_TO_DATE_UTC"
curl -sS -i -H "X-API-Key: $API_KEY" \
  "$BASE_URL/audit/encounters?from=$AUDIT_FROM_DATE_UTC&to=$AUDIT_TO_DATE_UTC" | sed -n '1,80p'
echo

echo "== Validation error: bad date (expect 400) =="
curl -sS -i \
  -H "Content-Type: application/json" \
  -H "X-API-Key: $API_KEY" \
  -X POST "$BASE_URL/encounters" \
  -d '{
    "patientId":"P999",
    "providerId":"T999",
    "encounterDate":"not-a-date",
    "encounterType":"follow_up",
    "clinicalData":{"note":"bad date"}
  }' | sed -n '1,80p'
echo

echo "Smoke tests finished."
