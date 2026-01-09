-- Migration: Grant all permissions to service_role
-- Date: 2026-01-03
-- Description: Fix permission denied errors by granting full access to service_role
--              This allows the C backend to use service_role key without RLS restrictions

-- ============================================================================
-- GRANT ALL PERMISSIONS TO SERVICE_ROLE
-- ============================================================================

-- Grant permissions on all existing tables
GRANT ALL ON ALL TABLES IN SCHEMA public TO service_role;

-- Grant permissions on all existing sequences (for auto-increment IDs)
GRANT ALL ON ALL SEQUENCES IN SCHEMA public TO service_role;

-- Grant permissions on all existing functions (for RPC calls)
GRANT ALL ON ALL FUNCTIONS IN SCHEMA public TO service_role;

-- ============================================================================
-- GRANT DEFAULT PERMISSIONS FOR FUTURE OBJECTS
-- ============================================================================
-- This ensures any new tables/sequences/functions automatically get permissions

ALTER DEFAULT PRIVILEGES IN SCHEMA public
GRANT ALL ON TABLES TO service_role;

ALTER DEFAULT PRIVILEGES IN SCHEMA public
GRANT ALL ON SEQUENCES TO service_role;

ALTER DEFAULT PRIVILEGES IN SCHEMA public
GRANT ALL ON FUNCTIONS TO service_role;

-- ============================================================================
-- VERIFY PERMISSIONS
-- ============================================================================
SELECT 
    schemaname,
    tablename,
    has_table_privilege('service_role', schemaname||'.'||tablename, 'INSERT') as can_insert,
    has_table_privilege('service_role', schemaname||'.'||tablename, 'SELECT') as can_select
FROM pg_tables
WHERE schemaname = 'public'
ORDER BY tablename;
