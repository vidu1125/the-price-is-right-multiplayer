#!/usr/bin/env python3
"""
Check Supabase database schema
"""
import sys
sys.path.insert(0, '/home/duyen/DAIHOC/NetworkProgramming/Project/the-price-is-right-multiplayer/Backend')

from app.models import db
from app.main import app

def check_table_schema(table_name):
    """Query information_schema to get table columns"""
    query = """
        SELECT 
            column_name,
            data_type,
            character_maximum_length,
            is_nullable,
            column_default
        FROM information_schema.columns
        WHERE table_name = %s
        ORDER BY ordinal_position;
    """
    
    with app.app_context():
        result = db.session.execute(db.text(query), {'table_name': table_name})
        rows = result.fetchall()
        
        if not rows:
            print(f"❌ Table '{table_name}' not found or no columns")
            return
        
        print(f"\n📋 Table: {table_name}")
        print("=" * 80)
        print(f"{'Column':<25} {'Type':<20} {'Nullable':<10} {'Default':<20}")
        print("-" * 80)
        
        for row in rows:
            col_name = row[0]
            col_type = row[1]
            max_len = row[2]
            nullable = row[3]
            default = row[4] or ''
            
            if max_len:
                col_type = f"{col_type}({max_len})"
            
            print(f"{col_name:<25} {col_type:<20} {nullable:<10} {default:<20}")

def main():
    print("🔍 CHECKING SUPABASE DATABASE SCHEMA")
    print("=" * 80)
    
    tables = ['accounts', 'rooms', 'room_members', 'matches', 'match_players']
    
    for table in tables:
        try:
            check_table_schema(table)
        except Exception as e:
            print(f"\n❌ Error checking {table}: {e}")
    
    print("\n" + "=" * 80)

if __name__ == "__main__":
    main()
