#!/usr/bin/env python3
"""
Script kiểm tra dữ liệu trong bảng questions của Supabase
"""
import requests
import json

SUPABASE_URL = "https://ecnfnieobrpfbosucbdz.supabase.co"
SUPABASE_KEY = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImVjbmZuaWVvYnJwZmJvc3VjYmR6Iiwicm9sZSI6InNlcnZpY2Vfcm9sZSIsImlhdCI6MTc2NDY3MDE0OSwiZXhwIjoyMDgwMjQ2MTQ5fQ.GpNNqaxwqC2-dCGjiYuGb2MsplCW4esT_w35kakC5Kg"

headers = {
    "apikey": SUPABASE_KEY,
    "Authorization": f"Bearer {SUPABASE_KEY}",
    "Content-Type": "application/json"
}

print("=" * 60)
print("CHECKING SUPABASE DATABASE")
print("=" * 60)

# 1. Lấy tất cả câu hỏi (không filter)
print("\n1. Tất cả câu hỏi trong bảng 'questions':")
print("-" * 60)
url = f"{SUPABASE_URL}/rest/v1/questions?select=*&limit=10"
resp = requests.get(url, headers=headers)
print(f"Status: {resp.status_code}")
if resp.status_code == 200:
    data = resp.json()
    print(f"Số lượng: {len(data)}")
    for i, q in enumerate(data[:5]):
        print(f"\n  [{i}] id={q.get('id')}")
        print(f"      type={q.get('type')}")
        print(f"      active={q.get('active')}")
        print(f"      data={json.dumps(q.get('data'), ensure_ascii=False)[:100]}...")
else:
    print(f"Error: {resp.text}")

# 2. Lấy câu hỏi với type=MCQ
print("\n2. Câu hỏi với type='MCQ':")
print("-" * 60)
url = f"{SUPABASE_URL}/rest/v1/questions?select=*&type=eq.MCQ&limit=5"
resp = requests.get(url, headers=headers)
print(f"Status: {resp.status_code}")
if resp.status_code == 200:
    data = resp.json()
    print(f"Số lượng: {len(data)}")
else:
    print(f"Error: {resp.text}")

# 3. Lấy câu hỏi với active=true
print("\n3. Câu hỏi với active=true:")
print("-" * 60)
url = f"{SUPABASE_URL}/rest/v1/questions?select=*&active=eq.true&limit=5"
resp = requests.get(url, headers=headers)
print(f"Status: {resp.status_code}")
if resp.status_code == 200:
    data = resp.json()
    print(f"Số lượng: {len(data)}")
else:
    print(f"Error: {resp.text}")

# 4. Kiểm tra các giá trị unique của type
print("\n4. Các giá trị unique của field 'type':")
print("-" * 60)
url = f"{SUPABASE_URL}/rest/v1/questions?select=type"
resp = requests.get(url, headers=headers)
if resp.status_code == 200:
    data = resp.json()
    types = set(q.get('type') for q in data if q.get('type'))
    print(f"Types found: {types}")
else:
    print(f"Error: {resp.text}")

# 5. Kiểm tra schema của bảng
print("\n5. Columns trong bảng questions:")
print("-" * 60)
url = f"{SUPABASE_URL}/rest/v1/questions?select=*&limit=1"
resp = requests.get(url, headers=headers)
if resp.status_code == 200:
    data = resp.json()
    if data:
        print(f"Columns: {list(data[0].keys())}")
    else:
        print("Bảng trống!")
else:
    print(f"Error: {resp.text}")

print("\n" + "=" * 60)
